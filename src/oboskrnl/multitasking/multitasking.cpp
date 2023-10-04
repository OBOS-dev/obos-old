/*
	multitasking.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <multitasking/multitasking.h>
#include <multitasking/thread.h>
#include <multitasking/mutex/mutexHandle.h>

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
#elif defined (__x86_64__)
#define isInKernelCode(frame) frame.rip >= 0xFFFFFFFF80000000
#define setFrameAddress(frame_addr) \
asm volatile("mov %0, %%rax" :\
	: "r"(frame_addr)\
	: "memory");
#endif

extern "C" char __etext;
extern "C" void attribute(noreturn) callSwitchToTaskImpl(PBYTE stack, PVOID pageMap, void(*switchToTaskImpl)(obos::multitasking::Thread* newThread), obos::multitasking::Thread* funcPar);

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
		Thread* g_currentThread = nullptr;
		bool g_initialized = false;
		MutexHandle* g_schedulerMutex = nullptr;
		SIZE_T g_timerTicks = 1;

		UINT16_T g_schedulerFrequency = 1000;

		bool canRanTask(Thread* thread)
		{
			return  thread->iterations < (UINT32_T)thread->priority &&
					thread->status == (UINT32_T)Thread::status_t::RUNNING;
		}

		extern void switchToTaskAsm();
		extern void switchToUserModeTask();

		static void switchToTaskImpl(Thread* newThread)
		{
			g_currentThread = newThread;
			if (g_currentThread->tid == 0)
			{
				if (g_currentThread->frame.intNumber != 0x30)
					SendEOI(g_currentThread->frame.intNumber - 32);
				g_schedulerMutex->Unlock();
				setFrameAddress(&g_currentThread->frame);
				switchToTaskAsm();
			}
			if (!g_currentThread->owner->isUserMode || g_currentThread->isServicingSyscall || isInKernelCode(g_currentThread->frame))
			{
				if (g_currentThread->frame.intNumber != 0x30)
					SendEOI(g_currentThread->frame.intNumber - 32);
				g_schedulerMutex->Unlock();
				setFrameAddress(&g_currentThread->frame);
				switchToTaskAsm();
			}
			SetTSSStack(reinterpret_cast<PBYTE>(g_currentThread->tssStackBottom) + 8192);
			if (g_currentThread->frame.intNumber != 0x30)
				SendEOI(g_currentThread->frame.intNumber - 32);
			g_schedulerMutex->Unlock();
			setFrameAddress(&g_currentThread->frame);
			switchToUserModeTask();
			kpanic(nullptr, nullptr, kpanic_format("Could not switch to task with pid %x, tid %x."), g_currentThread->owner->pid, g_currentThread->tid);
		}

		void switchToTask(Thread* newThread)
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
			kpanic(nullptr, nullptr, kpanic_format("Could not switch to task with pid %x, tid %x."), g_currentThread->owner->pid, g_currentThread->tid);
		}

#pragma GCC push_options
#pragma GCC optimize("O3")
		void findNewTask(const interrupt_frame* frame)
		{
			if (frame->intNumber != 0x30)
				g_timerTicks++;
			if (!g_initialized)
			{
				if (frame->intNumber != 0x30)
					SendEOI(frame->intNumber - 32);
				return;
			}
			if (!g_schedulerMutex->Lock(false))
				return;
			if (g_currentThread)
			{
				g_currentThread->frame = *frame;
				g_currentThread->lastTimeRan = g_timerTicks - 1;
			}
			else
			{
				if (frame->intNumber != 0x30)
					SendEOI(frame->intNumber - 32);
				return;
			}
			if (g_threads->len == 1)
				return;

			EnterKernelSection();
			Pic(Pic::PIC1_CMD, Pic::PIC1_DATA).disableIrq(0);
		findNew:

			Thread* threadToRun = nullptr;
			Thread* currentThread = nullptr;

			// Find the thread that's starving in a queue and run it.
			
			// Find a new task.
			for (int i = 3; i != -1; i--)
			{
				SIZE_T lowestTimeRan = 0;
				for (list_node_t* node = g_threadPriorityList[i]->head; node != nullptr; node = node->next)
				{
					currentThread = (Thread*)node->val;
					if (currentThread->status & (DWORD)Thread::status_t::BLOCKED)
					{
						if (currentThread->isBlockedCallback)
							if (currentThread->isBlockedCallback(const_cast<Thread*>(currentThread), currentThread->isBlockedUserdata))
								const_cast<Thread*>(currentThread)->status &= (~(DWORD)Thread::status_t::BLOCKED);
							else {}
						else
							currentThread->status = (DWORD)Thread::status_t::DEAD;
					}
					if (currentThread->status == (DWORD)Thread::status_t::RUNNING)
					{
						if (lowestTimeRan < currentThread->lastTimeRan)
						{
							lowestTimeRan = currentThread->lastTimeRan;
							threadToRun = currentThread;
						}
					}
				}
			}
			if (!threadToRun)
				goto findNew;

			LeaveKernelSection();
			Pic(Pic::PIC1_CMD, Pic::PIC1_DATA).enableIrq(0);

			if (threadToRun == g_currentThread)
			{
				if (frame->intNumber != 0x30)
					SendEOI(frame->intNumber - 32);
				g_schedulerMutex->Unlock();
				return;
			}
			switchToTask(const_cast<Thread*>(threadToRun));
		}
#pragma GCC pop_options

		void SetPITFrequency(UINT16_T freq)
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
			kmainThread->tid = 0;
#if defined(__i686__)
			kmainThread->frame.eip = GET_FUNC_ADDR(kmainThr);
			kmainThread->frame.esp = GET_FUNC_ADDR(&thrstack_top);
			kmainThread->frame.ebp = 0;
			kmainThread->frame.eflags = getEflags();
#else
			kmainThread->frame.rip = GET_FUNC_ADDR(kmainThr);
			kmainThread->frame.rsp = GET_FUNC_ADDR(&thrstack_top) + 8;
			kmainThread->frame.rbp = GET_FUNC_ADDR(&thrstack_top) + 8;
			kmainThread->frame.rflags = getEflags();
			UINTPTR_T* _thrstack_top = (UINTPTR_T*)&thrstack_top;
			_thrstack_top[-1] = 0;
#endif
			kmainThread->priority = Thread::priority_t::NORMAL;
			kmainThread->status = (UINT32_T)Thread::status_t::RUNNING;
			kmainThread->exitCode = 0;
			kmainThread->iterations = 0;
			kmainThread->lastError = 0;
			list_lpush(g_threads			  , list_node_new(kmainThread));
			list_lpush(g_threadPriorityList[2], list_node_new(kmainThread));

			g_schedulerMutex = new MutexHandle{};

			SetPITFrequency(g_schedulerFrequency);
			
			RegisterInterruptHandler(0x20, findNewTask);
			RegisterInterruptHandler(0x30, findNewTask);
			LeaveKernelSection();
			switchToTask(kmainThread);
		}
	}
}