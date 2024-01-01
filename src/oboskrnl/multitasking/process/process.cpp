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
			
			return ret;
		}
		bool ExitProcessCallback(thread::Thread*, Process* process)
		{
			process->vallocator.FreeUserProcessAddressSpace();
			thread::ExitThread(0);
		}
		bool TerminateProcess(Process *process)
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
			thread::Thread* currentThread = (thread::Thread*)thread::GetCurrentCpuLocalPtr()->currentThread;
			// Freeing the address space should comes last.
			if (currentThread->owner != process)
				process->vallocator.FreeUserProcessAddressSpace();
			else
				putThreadAtFunctionWithCPUTempStack(currentThread, (void*)ExitProcessCallback, currentThread, process);
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