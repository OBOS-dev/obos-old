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
#define isInKernelCode(frame) frame.eip >= 0xC0000000 && frame.eip < 0xE0000000
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

namespace obos
{
	extern void kmainThr();
	extern char thrstack_top;
	extern char stack_top;
	extern void SetTSSStack(PVOID);
	extern DWORD s_terminalColumn;
	extern DWORD s_terminalRow;
	namespace multitasking
	{
		list_t* g_threads = nullptr;
		list_t* g_threadPriorityList[4] = {
			nullptr, nullptr, nullptr, nullptr,
		};
		Thread* g_currentThread = nullptr;
		bool g_initialized = false;
		MutexHandle* g_schedulerMutex = nullptr;

		bool canRanTask(Thread* thread)
		{
			return  thread->iterations < (UINT32_T)thread->priority &&
					thread->status == (UINT32_T)Thread::status_t::RUNNING;
		}

		extern void switchToTaskAsm();
		extern void switchToUserModeTask();

		void switchToTask(Thread* newThread)
		{
			static Thread* s_newThread;
			s_newThread = newThread;
			if(g_currentThread)
#if defined(__i686__)
				if (s_newThread->owner->pageDirectory != memory::g_pageDirectory)
				{
					asm volatile("mov %0, %%esp;"
								 "mov %0, %%ebp" :
												 : "r"(&stack_top)
												 : "memory");
					// No more parameters past this point.
					s_newThread->owner->pageDirectory->switchToThis();
				}
#elif defined(__x86_64__)
				if (s_newThread->owner->level4PageMap != memory::g_level4PageMap)
				{
					asm volatile("mov %0, %%rsp;"
								 "mov %0, %%rbp" :
												 : "r"(&stack_top)
												 : "memory");
					s_newThread->owner->doContextSwitch();
				}
#endif
				g_currentThread = s_newThread;
			if (g_currentThread->tid == 0)
			{
				setFrameAddress(&g_currentThread->frame);
				switchToTaskAsm();
			}
			if (!g_currentThread->owner->isUserMode || g_currentThread->isServicingSyscall || isInKernelCode(g_currentThread->frame))
			{
				// If we're servicing a syscall, we need to set esp0 in the tss.
				if(g_currentThread->isServicingSyscall || isInKernelCode(g_currentThread->frame))
					SetTSSStack(reinterpret_cast<PBYTE>(g_currentThread->tssStackBottom) + 8192);
				
				switchToTaskAsm();
			}
			SetTSSStack(reinterpret_cast<PBYTE>(g_currentThread->tssStackBottom) + 8192);
			setFrameAddress(&g_currentThread->frame);
			switchToUserModeTask();
		}

		void findNewTask(const interrupt_frame* frame)
		{
			static BYTE s_cursorCounter = 0;
			static bool s_isCursorOn = false;
			static SIZE_T s_cursorRow = 0;
			static SIZE_T s_cursorColumn = 0;
			static bool s_cursorInitialized;
			if (s_cursorCounter++ == CURSOR_SLEEP_TIME_2_5_MS && CURSOR_SLEEP_TIME_2_5_MS != 0)
			{
				if (s_cursorRow != s_terminalRow)
				{
					if(s_cursorInitialized)
						ConsoleOutputCharacter(' ', s_cursorColumn, s_cursorRow);
					s_cursorRow = s_terminalRow;
				}
				s_cursorInitialized = true;
				if (s_cursorColumn != s_terminalColumn)
				{
					if (s_cursorColumn > s_terminalColumn)
						ConsoleOutputCharacter(' ', s_cursorColumn + 1, s_cursorRow);
					s_cursorColumn = s_terminalColumn;
				}
				if (s_isCursorOn && s_cursorRow == s_terminalRow)
				{
					ConsoleOutputCharacter('_', s_cursorColumn++, s_cursorRow);
					s_cursorColumn--;
				}
				else
					ConsoleOutputCharacter(' ', s_cursorColumn, s_cursorRow);
				s_isCursorOn = !s_isCursorOn;
				s_cursorCounter = 0;
			}
			if (!g_initialized)
			{
				if (frame->intNumber != 0x30)
					SendEOI(frame->intNumber - 32);
				return;
			}
			if (!g_schedulerMutex->Lock(false))
				return;
			if (g_currentThread)
				g_currentThread->frame = *frame;
			else
			{
				if (frame->intNumber != 0x30)
					SendEOI(frame->intNumber - 32);
				return;
			}
			
			// Tf all threads were ran, then clear the amount of iterations.

			EnterKernelSection();
		findNew:

			volatile Thread* currentThread = nullptr;
			for (int i = 0; i < 4 && !currentThread; i++)
			{
				list_iterator_t* iter = list_iterator_new(g_threadPriorityList[i], LIST_HEAD);
				for (list_node_t* node = list_iterator_next(iter); node != nullptr; node = list_iterator_next(iter))
				{
					currentThread = (Thread*)node->val;
					if (currentThread->status != (INT)Thread::status_t::RUNNING)
					{
						currentThread = nullptr;
						continue;
					}
					break;
				}
				list_iterator_destroy(iter);
			}
			if(currentThread)
			{
				if (currentThread->iterations >= (UINT32_T)currentThread->priority)
				{
					list_iterator_t* iter = list_iterator_new(g_threads, LIST_HEAD);
					for (list_node_t* node = list_iterator_next(iter); node != nullptr; node = list_iterator_next(iter))
					{
						currentThread = (Thread*)node->val;
						currentThread->iterations = 0;
					}
					list_iterator_destroy(iter);
				}
			}

			volatile list_node_t* currentNode = nullptr;

			// Find a new task.
			for (int i = 3; i > -1 && !currentNode; i--)
			{
				list_iterator_t* iter = list_iterator_new(g_threadPriorityList[i], LIST_TAIL);
				for (list_node_t* node = list_iterator_next(iter); node != nullptr; node = list_iterator_next(iter))
				{
					currentThread = (Thread*)node->val;
					if (utils::testBitInBitfield(currentThread->status, 2))
					{
						if (currentThread->isBlockedCallback)
							if (currentThread->isBlockedCallback(const_cast<Thread*>(currentThread), currentThread->isBlockedUserdata))
								utils::clearBitInBitfield(const_cast<Thread*>(currentThread)->status, 2);
							else {}
						else
							currentThread->status = (UINT32_T)Thread::status_t::DEAD;
					}
					if (canRanTask(const_cast<Thread*>(currentThread)))
					{
						currentNode = node;
						currentThread->iterations++;
						break;
					}
				}
				list_iterator_destroy(iter);
			}
			if (!currentNode && (utils::testBitInBitfield(g_currentThread->status, 2) || utils::testBitInBitfield(g_currentThread->status, 3) || utils::testBitInBitfield(g_currentThread->status, 0)))
				goto findNew;

			g_schedulerMutex->Unlock();

			LeaveKernelSection();
			if (frame->intNumber != 0x30)
				SendEOI(frame->intNumber - 32);
			if (currentThread == g_currentThread)
				return;
			switchToTask(const_cast<Thread*>(currentThread));
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
			kmainThread->frame.rsp = GET_FUNC_ADDR(&thrstack_top);
			kmainThread->frame.rbp = 0;
			kmainThread->frame.rflags = getEflags();
#endif
			kmainThread->priority = Thread::priority_t::NORMAL;
			kmainThread->status = (UINT32_T)Thread::status_t::RUNNING;
			kmainThread->exitCode = 0;
			kmainThread->iterations = 0;
			kmainThread->lastError = 0;
			list_lpush(g_threads			  , list_node_new(kmainThread));
			list_lpush(g_threadPriorityList[2], list_node_new(kmainThread));

			g_schedulerMutex = new MutexHandle{};

			// 400 hz, 2.5 ms. (1/400 * 1000)
			UINT32_T divisor = 1193180 / 400;
			
			outb(0x43, 0x36);
			
			UINT8_T l = (UINT8_T)(divisor & 0xFF);
			UINT8_T h = (UINT8_T)((divisor >> 8) & 0xFF);
			
			// Send the frequency divisor.
			outb(0x40, l);
			outb(0x40, h);
			
			RegisterInterruptHandler(0x20, findNewTask);
			RegisterInterruptHandler(0x30, findNewTask);
			LeaveKernelSection();
			switchToTask(kmainThread);
		}
	}
}