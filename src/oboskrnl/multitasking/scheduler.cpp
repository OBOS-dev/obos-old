/*
	oboskrnl/multitasking/scheduler.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <new>

#include <int.h>
#include <klog.h>
#include <atomic.h>

#include <multitasking/scheduler.h>
#include <multitasking/thread.h>
#include <multitasking/arch.h>
#include <multitasking/cpu_local.h>

#include <multitasking/locks/mutex.h>

#include <memory_manipulation.h>

#include <allocators/vmm/vmm.h>

#define getCPULocal() GetCurrentCpuLocalPtr()

namespace obos
{
	extern void kmain_common(byte* initrdDriverData, size_t initrdDriverSize);
	namespace thread
	{
		Thread::ThreadList g_priorityLists[4];
		uint64_t g_schedulerFrequency = 1000;
		uint64_t g_timerTicks = 0;
		uint64_t g_defaultAffinity = 0;
		locks::Mutex g_coreGlobalSchedulerLock;

		bool g_initialized = false;

#pragma GCC push_options
#pragma GCC optimize("O0")
#ifdef __GNUC__ 
#define DEFINE_IN_SECTION __attribute__((section(".sched_text")))
#else
#define DEFINE_IN_SECTION
#endif
		static bool DEFINE_IN_SECTION checkThreadAffinity(const Thread* thr)
		{
			return (thr->affinity >> GetCurrentCpuLocalPtr()->cpuId) & 1;
		}
		static bool DEFINE_IN_SECTION ThreadCanRun(const Thread* thr)
		{
			return (thr->status == THREAD_STATUS_CAN_RUN) &&
				checkThreadAffinity(thr) &&
				(thr->timeSliceIndex < thr->priority);
		}
		static Thread* DEFINE_IN_SECTION findRunnableThreadInList(Thread::ThreadList& list)
		{
			Thread* currentThread = list.tail;
			//volatile Thread* currentRunningThread = getCPULocal()->currentThread;
			Thread* ret = nullptr;

			while (currentThread)
			{
				bool clearTimeSliceIndex = currentThread->status & THREAD_STATUS_CLEAR_TIME_SLICE_INDEX;

				if (currentThread->timeSliceIndex >= currentThread->priority)
					currentThread->status |= THREAD_STATUS_CLEAR_TIME_SLICE_INDEX;

				if (ThreadCanRun(currentThread))
				{
					if (!ret)
						ret = currentThread;
					else
						if (currentThread->lastTimePreempted < ret->lastTimePreempted)
							ret = currentThread;
				}

				if (clearTimeSliceIndex)
				{
					currentThread->status &= ~THREAD_STATUS_CLEAR_TIME_SLICE_INDEX;
					currentThread->timeSliceIndex = 0;
				}
				
				currentThread = currentThread->prev_run;
			}

			return ret;
		}
		static Thread::ThreadList& DEFINE_IN_SECTION findThreadPriorityList()
		{
			Thread::ThreadList* list = nullptr;
			int i;
			for (i = 3; i > -1; i--)
			{
				constexpr thrPriority priorityTable[4] = {
					THREAD_PRIORITY_IDLE,
					THREAD_PRIORITY_LOW,
					THREAD_PRIORITY_NORMAL,
					THREAD_PRIORITY_HIGH,
				};
				thrPriority priority = priorityTable[i];
				list = &g_priorityLists[i];
				if (list->size > 0)
				{
					if (g_priorityLists[i].iterations < (int)priority)
						break;
				}
			}
			if (g_priorityLists[0].iterations >= (int)THREAD_PRIORITY_IDLE)
			{
				for (int j = 0; j < 4; j++)
				{
					while (atomic_test(&g_priorityLists[j].lock));
					atomic_set(&g_priorityLists[j].lock);
					g_priorityLists[j].iterations = 0;
					atomic_clear(&g_priorityLists[j].lock);
				}
				return findThreadPriorityList();
			}
			while (atomic_test(&g_priorityLists[i].lock));
			atomic_set(&g_priorityLists[i].lock);
			g_priorityLists[i].iterations++;
			atomic_clear(&g_priorityLists[i].lock);
			return *list;
		}
		static void DEFINE_IN_SECTION callBlockCallbacksOnList(Thread::ThreadList& list)
		{
			Thread* thread = list.head;
			while(thread)
			{
				if (thread->status & THREAD_STATUS_BLOCKED)
				{
					while (thread->flags & THREAD_FLAGS_CALLING_BLOCK_CALLBACK);
					OBOS_ASSERTP(!(thread->status & THREAD_STATUS_RUNNING), "Thread (tid %d) is both blocked and running (status 0x%e%X)!\n","", thread->tid, 4, thread->status);
					thread->flags |= THREAD_FLAGS_CALLING_BLOCK_CALLBACK;
					bool ret = callBlockCallbackOnThread(&thread->context, (bool(*)(void*,void*))thread->blockCallback.callback, thread, thread->blockCallback.userdata);
					thread->flags &= ~THREAD_FLAGS_CALLING_BLOCK_CALLBACK;
					if (ret)
						thread->status &= ~THREAD_STATUS_BLOCKED;
				}

				thread = thread->next_run;
			}
		}
		static size_t DEFINE_IN_SECTION getCpusInScheduler()
		{
			size_t nCpusInScheduler = 0;
			for (size_t i = 0; i < g_nCPUs; i++)
				nCpusInScheduler += g_cpuInfo[i].schedulerLock;
			return nCpusInScheduler;
		}
		void DEFINE_IN_SECTION schedule()
		{
			if(getCPULocal()->cpuId == 0)
				g_timerTicks++;
			volatile Thread* currentThread = getCPULocal()->currentThread;
			if (getCPULocal()->schedulerLock)
				return;

			lock:
			if (!g_coreGlobalSchedulerLock.Lock(0, false))
			{
				if (!getCpusInScheduler())
				{
					g_coreGlobalSchedulerLock.Unlock();
					goto lock;
				}
				auto switchToIdleTask = [&]() {
					currentThread->lastTimePreempted = g_timerTicks;
					currentThread->status |= THREAD_STATUS_CAN_RUN;
					currentThread->status &= ~THREAD_STATUS_RUNNING;
					currentThread->affinity = currentThread->ogAffinity;
					Thread* newThread = getCPULocal()->idleThread;
					newThread->status = (newThread->status & ~THREAD_STATUS_CAN_RUN) | THREAD_STATUS_RUNNING;
					getCPULocal()->currentThread = newThread;
					switchToThreadImpl(&newThread->context, newThread);
					return;
					};
				if (currentThread)
					if (ThreadCanRun((thread::Thread*)currentThread))
						return;
					else
						switchToIdleTask();
				else {}
				return;
			}
			
			atomic_set((bool*)&getCPULocal()->schedulerLock);

			//OBOS_ASSERTP(getCpusInScheduler() < 2, "");
			if (getCpusInScheduler() != 1)
			{
				atomic_clear((bool*)&getCPULocal()->schedulerLock);
				goto lock;
			}

			if (currentThread)
			{
				currentThread->lastTimePreempted = g_timerTicks;
				if (!(currentThread->status & THREAD_STATUS_DEAD))
					currentThread->status |= THREAD_STATUS_CAN_RUN;
				currentThread->status &= ~THREAD_STATUS_RUNNING;
				currentThread->affinity = currentThread->ogAffinity;
			}

			callBlockCallbacksOnList(g_priorityLists[3]);
			callBlockCallbacksOnList(g_priorityLists[2]);
			callBlockCallbacksOnList(g_priorityLists[1]);
			callBlockCallbacksOnList(g_priorityLists[0]);

			Thread::ThreadList* list = &findThreadPriorityList();
			Thread* newThread = nullptr;
			int foundHighPriority = 0;
		find:
			if (!list)
				list = &findThreadPriorityList();
			if (!list)
			{
				newThread = getCPULocal()->idleThread;
				goto found;
			}
			newThread = findRunnableThreadInList(*list);
			found:
			if (!newThread)
			{
				foundHighPriority += list == &g_priorityLists[3];
				list = list->prevThreadList;
				if(foundHighPriority < 2)
					goto find;
			}
			if (foundHighPriority == 2)
				newThread = getCPULocal()->idleThread;
			OBOS_ASSERTP(!(newThread->status & THREAD_STATUS_BLOCKED), "Thread (tid %d) is both blocked and is trying to be run! Status 0x%e%X\n", "", newThread->tid, 4, newThread->status);
			OBOS_ASSERTP(!(newThread->status & THREAD_STATUS_PAUSED), "Thread (tid %d) is both paused and is trying to be run! Status 0x%e%X\n", "", newThread->tid, 4, newThread->status);
			if (newThread != currentThread)
				if ((newThread->status & THREAD_STATUS_RUNNING))
					goto find;
			OBOS_ASSERTP(!inSchedulerFunction(newThread), "Thread (tid %d) was preempted while in the scheduler!\n", "", newThread->tid);
			if (newThread == currentThread)
			{
				currentThread->affinity = ((uint64_t)1 << getCPULocal()->cpuId);
				currentThread->status = (currentThread->status & ~THREAD_STATUS_CAN_RUN) | THREAD_STATUS_RUNNING;
				g_coreGlobalSchedulerLock.Unlock();
				atomic_clear((bool*)&getCPULocal()->schedulerLock);
				return;
			}
			if (!newThread)
				newThread = getCPULocal()->idleThread;
			newThread->affinity = ((uint64_t)1 << getCPULocal()->cpuId);
			getCPULocal()->currentThread = newThread;
			newThread->timeSliceIndex = newThread->timeSliceIndex + 1;
			newThread->status = (newThread->status & ~THREAD_STATUS_CAN_RUN) | THREAD_STATUS_RUNNING;
			g_coreGlobalSchedulerLock.Unlock();
			atomic_clear((bool*)&getCPULocal()->schedulerLock);
			switchToThreadImpl((taskSwitchInfo*)&newThread->context, newThread);
		}
#pragma GCC pop_options

		void InitializeScheduler()
		{
			new (&g_coreGlobalSchedulerLock) locks::Mutex{ false };
		
			memory::VirtualAllocator valloc{ nullptr };

			Thread* kernelMainThread = new Thread{};

			kernelMainThread->tid = g_nextTid++;
			kernelMainThread->status = THREAD_STATUS_RUNNING;
			kernelMainThread->priority = THREAD_PRIORITY_NORMAL;
			kernelMainThread->priorityList = g_priorityLists + 2;
			kernelMainThread->threadList = new Thread::ThreadList;
			
			setupThreadContext(&kernelMainThread->context, &kernelMainThread->stackInfo, (uintptr_t)kmain_common, 0, 0x10000, &valloc, nullptr);

			kernelMainThread->threadList->head = kernelMainThread;
			kernelMainThread->threadList->tail = kernelMainThread;
			kernelMainThread->threadList->size = 1;

			g_priorityLists[2].head = g_priorityLists[2].tail = kernelMainThread;
			g_priorityLists[2].size++;
			
			g_priorityLists[3].prevThreadList = g_priorityLists + 2;
			g_priorityLists[2].prevThreadList = g_priorityLists + 1;
			g_priorityLists[1].prevThreadList = g_priorityLists + 0;
			g_priorityLists[0].prevThreadList = g_priorityLists + 3;

			g_priorityLists[3].nextThreadList = g_priorityLists + 0;
			g_priorityLists[2].nextThreadList = g_priorityLists + 3;
			g_priorityLists[1].nextThreadList = g_priorityLists + 2;
			g_priorityLists[0].nextThreadList = g_priorityLists + 1;

			if (!StartCPUs())
				logger::panic(nullptr, "Could not start the other cores.");
			
			for (size_t i = 0; i < g_nCPUs; i++)
				g_defaultAffinity |= (uint64_t)1 << g_cpuInfo[i].cpuId;
			kernelMainThread->affinity = kernelMainThread->ogAffinity = /*g_defaultAffinity*/1;

			for (size_t i = 0; i < g_nCPUs; i++)
			{
				auto &idleThread = g_cpuInfo[i].idleThread;
				idleThread = new Thread{};
				idleThread->tid = g_nextTid++;
				idleThread->priorityList = g_priorityLists + 0;
				idleThread->status = THREAD_STATUS_CAN_RUN;
				idleThread->priority = THREAD_PRIORITY_IDLE;
				idleThread->affinity = idleThread->ogAffinity = (uint64_t)1 << g_cpuInfo[i].cpuId;
				auto threadListProc = kernelMainThread->threadList;
				if (threadListProc->tail)
					threadListProc->tail->next_list = idleThread;
				if(!threadListProc->head)
					threadListProc->head = idleThread;
				idleThread->prev_list = threadListProc->tail;
				threadListProc->tail = idleThread;
				threadListProc->size++;
				auto &priorityList = g_priorityLists[0];
				if (priorityList.tail)
					priorityList.tail->next_list = idleThread;
				if(!priorityList.head)
					priorityList.head = idleThread;
				idleThread->next_run = priorityList.tail;
				priorityList.tail = idleThread;
				priorityList.size++;
				setupThreadContext(&idleThread->context, &idleThread->stackInfo, (uintptr_t)idleTask, 0, 0x2000, &valloc, nullptr);
			}

			volatile Thread*& currentThread = getCPULocal()->currentThread;
			currentThread = (volatile Thread*)kernelMainThread;

			setupTimerInterrupt();
			thread::g_initialized = true;
			switchToThreadImpl((taskSwitchInfo*)&currentThread->context, (Thread*)currentThread);
			logger::panic(nullptr, "Could not switch to thread context for the kernel main thread while initializing the scheduler.");
		}

		cpu_local* GetCurrentCpuLocalPtr()
		{
			return (cpu_local*)getCurrentCpuLocalPtr();
		}
	}
}