/*
	oboskrnl/process/process.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <error.h>

#include <multitasking/thread.h>
#include <multitasking/scheduler.h>
#include <multitasking/cpu_local.h>

#include <multitasking/threadAPI/thrHandle.h>

#include <multitasking/process/process.h>
#include <multitasking/process/arch.h>

namespace obos
{
	namespace thread
	{
		extern void schedule();
	}
	namespace process
	{
		Process::ProcessList g_processes;
		Process* CreateProcess(bool isUsermode)
		{
			Process* ret = new Process{};

			setupContextInfo(&ret->context);
			ret->isUsermode = isUsermode;
			ret->parent = (process::Process*)((thread::cpu_local*)thread::getCurrentCpuLocalPtr())->currentThread->owner;
			if (g_processes.tail)
			{
				g_processes.tail->next = ret;
				ret->prev = g_processes.tail;
			}
			if (!g_processes.head)
				g_processes.head = ret;
			g_processes.tail = ret;
			ret->pid = g_processes.size++;
			
			if (ret->parent->children.tail)
			{
				ret->parent->children.tail->next_child = ret;
				ret->prev_child = ret->parent->children.tail;
			}
			if (!ret->parent->children.head)
				ret->parent->children.head = ret;
			ret->parent->children.tail = ret;
			ret->parent->children.size++;
			ret->console = &g_kernelConsole;
			
			return ret;
		}
#ifdef __x86_64__
		extern "C" bool _Impl_ExitProcessCallback();
		asm(
			".intel_syntax noprefix;"
			".extern _ZN4obos11SetTSSStackEPv;"
			".extern _ZN4obos11SetTSSStackEPv;"
			".extern _ZN4obos7process19ExitProcessCallbackEPNS_6thread6ThreadEPNS0_7ProcessE;"
			".global _Impl_ExitProcessCallback;"
			"_Impl_ExitProcessCallback:;"
			"	push rdi;"
			"	push rsi;"
			"	lea rdi, [rsp+0x10];"
			"	call _ZN4obos6SetISTEPv;"
			"	lea rdi, [rsp+0x10];"
			"	call _ZN4obos11SetTSSStackEPv;"
			"	pop rsi;"
			"	pop rdi;"
			"	call _ZN4obos7process19ExitProcessCallbackEPNS_6thread6ThreadEPNS0_7ProcessE;"
			"	jmp $;"
			".att_syntax prefix;"
		);
#endif
		bool ExitProcessCallback(thread::Thread* thr, Process* process)
		{
			process->vallocator.FreeUserProcessAddressSpace();
			bool isThrDead = thr->status == thread::THREAD_STATUS_DEAD;
			if (thr->flags & thread::THREAD_FLAGS_IS_EXITING_PROCESS && !isThrDead)
			{
				thr->exitCode = 0;
				thr->status = thread::THREAD_STATUS_DEAD;
				thr->flags = 0;
				if (thr->prev_run)
					thr->prev_run->next_run = thr->next_run;
				if (thr->next_run)
					thr->next_run->prev_run = thr->prev_run;
				if (thr->priorityList->head == thr)
					thr->priorityList->head = thr->next_run;
				if (thr->priorityList->tail == thr)
					thr->priorityList->tail = thr->prev_run;
				thr->priorityList->size--;
				if (!thr->references)
				{
					if (thr->prev_list)
						thr->prev_list->next_list = thr->next_list;
					if (thr->next_list)
						thr->next_list->prev_list = thr->prev_list;
					if (thr->threadList->head == thr)
						thr->threadList->head = thr->next_list;
					if (thr->threadList->tail == thr)
						thr->threadList->tail = thr->prev_list;
					thr->threadList->size--;
					delete thr;
				}
			}
			thread::startTimer(0);
			thread::callScheduler(false);
			while (1);
			return false;
		}
		bool TerminateProcess(Process *process, bool isCurrentProcess)
		{
			bool found = false;
			for (auto node = g_processes.head; node; )
			{
				if (node == process)
				{
					found = true;
					break;
				}

				node = node->next;
			}
			if (!found)
			{
				SetLastError(OBOS_ERROR_NO_SUCH_OBJECT);
				return false;
			}
			// Terminate all threads.
			for (auto _thread = process->threads.head; _thread;)
			{
				thread::ThreadHandle thr;
				thr.OpenThread(_thread->tid);
				_thread = _thread->next_list; // Get the next thread before the current thread gets freed.
				thr.TerminateThread(0);
				thr.CloseHandle();
			}
			freeProcessContext(&process->context);
			thread::Thread* currentThread = (thread::Thread*)thread::GetCurrentCpuLocalPtr()->currentThread;
			// Freeing the address space should comes last.
			if (currentThread->owner != process && !isCurrentProcess /* sometimes currentThread->owner can be nullptr because of ExitThread calling this function. */)
				process->vallocator.FreeUserProcessAddressSpace();
			else
			{
				currentThread->flags |= thread::THREAD_FLAGS_IS_EXITING_PROCESS;
#ifndef __x86_64__
				putThreadAtFunctionWithCPUTempStack(currentThread, (void*)ExitProcessCallback, currentThread, process);
#else
				putThreadAtFunctionWithCPUTempStack(currentThread, (void*)_Impl_ExitProcessCallback, currentThread, process);
#endif
			}
			return true; // Only reached if currentThread->owner != process
		}
		bool GracefullyTerminateProcess(Process *process)
		{
			bool found = false;
			for (auto node = g_processes.head; node; )
			{
				if (node == process)
				{
					found = true;
					break;
				}

				node = node->next;
			}
			if (!found)
			{
				SetLastError(OBOS_ERROR_NO_SUCH_OBJECT);
				return false;
			}
			thread::Thread* signalerThread = nullptr;
			if (process->threads.head)
			{
				CallSignal(signalerThread = process->threads.head, SIGTERM);
			}
			else
			{
				if (process->signal_table[SIGTERM])
				{
					thread::ThreadHandle thr;
					thr.CreateThread(
						thread::THREAD_PRIORITY_NORMAL,
						0,
						(void(*)(uintptr_t))process->signal_table[SIGTERM],
						0,
						thread::g_defaultAffinity,
						process,
						false
						);
					signalerThread = (thread::Thread*)thr.GetUnderlyingObject();
					thr.CloseHandle();
				}
			}
			while (signalerThread->flags & thread::THREAD_FLAGS_IN_SIGNAL);
			TerminateProcess(process);
			return true;
		}
	}
}