/*
	multitasking.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <multitasking/multitasking.h>
#include <multitasking/thread.h>

#include <utils/memory.h>

#include <inline-asm.h>

#include <descriptors/idt/idt.h>
#include <descriptors/idt/pic.h>

#include <console.h>

#if defined(__i686__)
#define isInKernelCode(frame) ((frame.eip >= 0xC0000000) && (frame.eip < 0xE0000000))
#define setFrameAddress(frame_addr) \
asm volatile("mov %0, %%eax" :\
	: "r"(frame_addr)\
	: "memory");
#define FRAME_IP eip
#elif defined (__x86_64__)
#define FRAME_IP rip
#define isInKernelCode(frame) frame.rip >= 0xFFFFFFFF80000000
#define setFrameAddress(frame_addr) \
asm volatile("mov %0, %%rax" :\
	: "r"(frame_addr)\
	: "memory");
#endif

#define inRange(val, rStart, rEnd) (((UINTPTR_T)(val)) >= ((UINTPTR_T)(rStart)) && ((UINTPTR_T)(val)) < ((UINTPTR_T)(rEnd)))

extern "C" char __etext;
extern "C" char _scheduler_text_start;
extern "C" char _scheduler_text_end;
extern "C" void attribute(noreturn) callSwitchToTaskImpl(PBYTE stack, PVOID pageMap, void(*switchToTaskImpl)(obos::multitasking::Thread* newThread), obos::multitasking::Thread* funcPar);
extern "C" bool callBlockCallback(PBYTE stack, PVOID pageMap, bool(*callback)(obos::multitasking::Thread*, PVOID), obos::multitasking::Thread* funcPar1, PVOID funcPar2);
extern "C" void idleTask(PVOID);

namespace obos
{
	extern void kmainThr();
	extern char thrstack_top;
	extern char stack_top;
	extern char stack_bottom;
	extern void SetTSSStack(PVOID);
	namespace multitasking
	{
		list_t* g_threads = nullptr;
		list_t* g_threadPriorityList[4] = {
			nullptr, nullptr, nullptr, nullptr,
		};
		int g_threadPriorityListIterations[4] = {
			0,0,0,0,
		};
		Thread* g_currentThread = nullptr;
		bool g_initialized = false;
		struct lock
		{
			bool locked;
			bool Lock(bool)
			{
				if (locked)
					return false;
				locked = true;
				return true;
			}
			void Unlock()
			{
				locked = false;
			}
		} g_schedulerMutex;
		SIZE_T g_timerTicks = 1;

		UINT32_T g_schedulerFrequency = 1000;

		bool canRanTask(Thread* thread)
		{
			return  thread->iterations < (UINT32_T)thread->priority &&
					thread->status == (UINT32_T)Thread::status_t::RUNNING;
		}

		extern void switchToTaskAsm();
		extern void switchToUserModeTask();

		static void attribute(section(".scheduler_text")) switchToTaskImpl(Thread* newThread)
		{
			// We must disable interrupts here!
			asm volatile("cli");
			g_currentThread = newThread;
			if (g_currentThread->tid == 0)
			{
				if (g_currentThread->frame.intNumber != 0x30)
					SendEOI(g_currentThread->frame.intNumber - 32);
				g_schedulerMutex.Unlock();
				setFrameAddress(&g_currentThread->frame);
				switchToTaskAsm();
			}
			if (!g_currentThread->owner->isUserMode || g_currentThread->isServicingSyscall || isInKernelCode(g_currentThread->frame))
			{
				if (g_currentThread->frame.intNumber != 0x30)
					SendEOI(g_currentThread->frame.intNumber - 32);
				g_schedulerMutex.Unlock();
				setFrameAddress(&g_currentThread->frame);
				switchToTaskAsm();
			}
			SetTSSStack(reinterpret_cast<PBYTE>(g_currentThread->tssStackBottom) + 8192);
			if (g_currentThread->frame.intNumber != 0x30)
				SendEOI(g_currentThread->frame.intNumber - 32);
			g_schedulerMutex.Unlock();
			setFrameAddress(&g_currentThread->frame);
			switchToUserModeTask();
			kpanic(nullptr, nullptr, kpanic_format("Could not switch to task with pid %p, tid %p."), g_currentThread->owner->pid, g_currentThread->tid);
		}

		void attribute(section(".scheduler_text")) switchToTask(Thread* newThread)
		{
			alignas(4096) static BYTE stack[4096];
			UINTPTR_T* pageMap = nullptr;
#if defined(__x86_64__)
			if (newThread->owner)
			{
				memory::g_level4PageMap = newThread->owner->level4PageMap;
				pageMap = newThread->owner->level4PageMap->getPageMap();
			}
			else
				pageMap = memory::g_kernelPageMap.getPageMap();
#elif defined(__i686__)
			if(newThread->owner)
			{
				memory::g_pageDirectory = newThread->owner->pageDirectory;
				pageMap = newThread->owner->pageDirectory->getPageDirectory();
			}
			else
				pageMap = memory::g_pageDirectory->getPageDirectory();
#endif
			callSwitchToTaskImpl(stack + 4096, pageMap, switchToTaskImpl, newThread);
			kpanic(nullptr, nullptr, kpanic_format("Could not switch to task with pid %p, tid %p."), g_currentThread->owner->pid, g_currentThread->tid);
		}


		bool attribute(section(".scheduler_text")) callBlockCallbackOnThread(Thread* thread)
		{
			if (!thread->isBlockedCallback)
				return false;

			static BYTE block_callback_stack[8192];
			UINTPTR_T* pageMap = nullptr;
#if defined(__x86_64__)
			auto oldPageMap = memory::g_level4PageMap;
			if (thread->owner)
			{
				memory::g_level4PageMap = thread->owner->level4PageMap;
				pageMap = thread->owner->level4PageMap->getPageMap();
			}
			else
				pageMap = memory::g_kernelPageMap.getPageMap();
#elif defined(__i686__)
			auto oldPageMap = memory::g_pageDirectory;
			if (newThread->owner)
			{
				memory::g_pageDirectory = thread->owner->pageDirectory;
				pageMap = thread->owner->pageDirectory->getPageDirectory();
			}
			else
				pageMap = memory::g_pageDirectory->getPageDirectory();
#endif
			bool ret = 
				callBlockCallback(block_callback_stack + sizeof(block_callback_stack),
								pageMap, 
								thread->isBlockedCallback, 
								thread, thread->isBlockedUserdata);

#if defined(__x86_64__)
			memory::g_level4PageMap = oldPageMap;
#elif defined(__i686__)
			memory::g_pageDirectory = oldPageMap;
#endif

			return ret;
		}
		Thread* attribute(section(".scheduler_text")) findTaskInPriorityList(list_t* list)
		{
			if (!list->head)
				return nullptr;
			Thread* currentThread = (Thread*)list->head->val;
			Thread* ret = nullptr;
			for (list_node_t* node = list->tail; node != nullptr; )
			{
				currentThread = (Thread*)node->val;

				bool clearIterations = (currentThread->status & Thread::status_t::CLEAR_ITERATIONS) != 0;

				if (currentThread->iterations == (DWORD)currentThread->priority)
					currentThread->status |= Thread::status_t::CLEAR_ITERATIONS;

				if (currentThread->status == Thread::status_t::RUNNING && (currentThread->iterations < (SIZE_T)currentThread->priority))
					if (!ret || currentThread->lastTimeRan < ret->lastTimeRan)
						ret = currentThread;

				if (clearIterations)
				{
					currentThread->iterations = 0;
					currentThread->status &= ~Thread::status_t::CLEAR_ITERATIONS;
				}

				node = node->prev;
			}
			
			if(ret)
				if (ret->status != Thread::status_t::RUNNING)
					ret = nullptr;
			
			return ret;
		}
		list_t* attribute(section(".scheduler_text")) getCurrentPriorityList()
		{
			list_t* list = nullptr;
			int i;
			for (i = 3; i > -1; i--)
			{
				constexpr Thread::priority_t priorityTable[4] = {
					Thread::priority_t::IDLE,
					Thread::priority_t::LOW,
					Thread::priority_t::NORMAL,
					Thread::priority_t::HIGH,
				};
				Thread::priority_t priority = priorityTable[i];
				list = g_threadPriorityList[i];
				if (list->len > 0)
				{
					if (g_threadPriorityListIterations[i] < (int)priority)
						break;
				}
			}
			if (g_threadPriorityListIterations[0] >= (int)Thread::priority_t::IDLE)
			{
				for (int i = 0; i < 4; i++)
					g_threadPriorityListIterations[i] = 0;
				list = getCurrentPriorityList();
				return list;
			}
			g_threadPriorityListIterations[i]++;
			return list;
		}
		Thread* attribute(section(".scheduler_text")) findNewTask()
		{
			EnterKernelSection();

			/*Thread* currentTask = findTaskInPriorityList(g_threadPriorityList[3]);

			for (int i = 2; i != -1; i--)
			{
				Thread* chosenOne = findTaskInPriorityList(g_threadPriorityList[i]);
				if (!chosenOne)
					continue;
				if (!currentTask)
				{
					currentTask = chosenOne;
					continue;
				}
				if (chosenOne->lastTimeRan < currentTask->lastTimeRan)
					currentTask = chosenOne;
			}*/
			Thread* currentTask = findTaskInPriorityList(getCurrentPriorityList());

			LeaveKernelSection();
			if (!currentTask)
			{
				currentTask = (Thread*)g_threads->head->next->val; // The idle task.
				return currentTask;
			}
			if (g_currentThread->frame.intNumber != 0x30)
				currentTask->lastTimeRan = g_timerTicks - 1;
			currentTask->iterations++;
			return currentTask;
		}
		void attribute(section(".scheduler_text")) schedule(const interrupt_frame* frame)
		{
			if (frame->intNumber != 0x30)
				g_timerTicks++;
			if (!g_initialized)
			{
				if (frame->intNumber != 0x30)
					SendEOI(frame->intNumber - 32);
				return;
			}
			if (!g_schedulerMutex.Lock(false))
			{
				if (frame->intNumber != 0x30)
					SendEOI(frame->intNumber - 32);
				return;
			}
			if (inRange(frame->FRAME_IP, &_scheduler_text_start, &_scheduler_text_end))
			{
				g_schedulerMutex.Unlock();
				if (frame->intNumber != 0x30)
					SendEOI(frame->intNumber - 32);
				return;
			}
			if (g_currentThread)
			{
				g_currentThread->frame = *frame;	
			}
			else
			{
				if (frame->intNumber != 0x30)
					SendEOI(frame->intNumber - 32);
				g_schedulerMutex.Unlock();
				return;
			}
			if (g_threads->len == 1)
			{
				if (frame->intNumber != 0x30)
					SendEOI(frame->intNumber - 32);
				g_schedulerMutex.Unlock();
				return;
			}

			// Call all threads' block callbacks
			for (list_node_t* node = g_threads->head; node != nullptr;)
			{
				Thread* currentThread = (Thread*)node->val;

				if (currentThread->status & Thread::status_t::BLOCKED)
					if (callBlockCallbackOnThread(currentThread))
						currentThread->status &= ~Thread::status_t::BLOCKED;

				node = node->next;
			}

			Thread* thread = findNewTask();
			if (g_currentThread == thread)
			{
				if (frame->intNumber != 0x30)
					SendEOI(frame->intNumber - 32);
				g_schedulerMutex.Unlock();
				return;
			}

			switchToTask(thread);
		}

		void SetPITFrequency(UINT32_T freq)
		{
			UINT32_T divisor = 1193182 / freq;

			outb(0x43, 0x36);

			UINT8_T l = (UINT8_T)(divisor & 0xFF);
			UINT8_T h = (UINT8_T)((divisor >> 8) & 0xFF);

			// Send the frequency divisor.
			outb(0x40, l);
			outb(0x40, h);
		}
		void DisablePIT()
		{
			// Disable channel 0 of the PIT.
			outb(0x43, 0b00000000);
		}

		void InitializeMultitasking()
		{
			EnterKernelSection();
			g_threads = list_new();
			for(int i = 0; i < 4; g_threadPriorityList[i++] = list_new());
			Thread* kmainThread = new Thread{};
			Thread* idleThread = new Thread{};
			kmainThread->tid = g_nextThreadTid++;
#if defined(__i686__)
			kmainThread->frame.eip = GET_FUNC_ADDR(kmainThr);
			kmainThread->frame.esp = GET_FUNC_ADDR(&thrstack_top);
			kmainThread->frame.ebp = 0;
			kmainThread->frame.eflags = getEflags();
#else
			kmainThread->frame.rip = GET_FUNC_ADDR(kmainThr);
			kmainThread->frame.rsp = GET_FUNC_ADDR(&thrstack_top) - 8;
			kmainThread->frame.rbp = 0;
			kmainThread->frame.rflags = getEflags();
			UINTPTR_T* _thrstack_top = (UINTPTR_T*)&thrstack_top;
			_thrstack_top[-1] = 0;
#endif
			kmainThread->priority = Thread::priority_t::NORMAL;
			kmainThread->status = (UINT32_T)Thread::status_t::RUNNING;
			kmainThread->exitCode = 0;
			kmainThread->iterations = 0;
			kmainThread->lastError = 0;
			list_rpush(g_threads			  , list_node_new(kmainThread));
			list_rpush(g_threadPriorityList[2], list_node_new(kmainThread));
			idleThread->tid = g_nextThreadTid++;
#if defined(__i686__)
			idleThread->frame.eip = GET_FUNC_ADDR(kmainThr);
			idleThread->frame.esp = GET_FUNC_ADDR(&thrstack_top);
			idleThread->frame.ebp = 0;
			idleThread->frame.eflags = getEflags();
#else
			idleThread->frame.rip = GET_FUNC_ADDR(idleTask);
			idleThread->frame.rsp = GET_FUNC_ADDR(&stack_top);
			idleThread->frame.rbp = 0;
			idleThread->frame.rflags = getEflags();
#endif
			idleThread->priority = Thread::priority_t::IDLE;
			idleThread->status = (UINT32_T)Thread::status_t::RUNNING;
			idleThread->exitCode = 0;
			idleThread->iterations = 0;
			idleThread->lastError = 0;
			list_rpush(g_threads, list_node_new(idleThread));
			list_rpush(g_threadPriorityList[0], list_node_new(idleThread));
			
			SetPITFrequency(g_schedulerFrequency);
			
			RegisterInterruptHandler(0x20, schedule);
			RegisterInterruptHandler(0x30, schedule);
			LeaveKernelSection();
			switchToTask(kmainThread);
		}
	}
}