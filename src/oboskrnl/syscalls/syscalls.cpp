/*
	oboskrnl/syscalls/syscalls.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <syscalls/syscalls.h>

#include <utils/memory.h>

#include <types.h>
#include <console.h>

#include <multitasking/multitasking.h>
#include <multitasking/thread.h>
#include <multitasking/threadHandle.h>

#include <process/process.h>

#include <memory_manager/paging/allocate.h>

extern int do_nothing();

#define DEFINE_RESERVED_PARAMETERS PVOID parameters

static bool checkUserPtr(PCVOID ptr, bool isAllocCall = false)
{
	UINTPTR_T iPtr = reinterpret_cast<UINTPTR_T>(ptr);
	if (iPtr < 0x401000)
		return false;
	if (iPtr >= 0xC0000000 && iPtr < 0xE0000000)
		return false;
	if (iPtr >= 0xFFCFF000)
		return false;
	if (!obos::memory::HasVirtualAddress(ptr, 1))
		return isAllocCall;
	return true;
}
static bool checkUserHandle(HANDLE handle)
{
	return (handle >= 0xC0000000 && handle < 0xE0000000) && (obos::memory::HasVirtualAddress(reinterpret_cast<PVOID>(handle), 1));
}

namespace obos
{
	namespace process
	{
		static void ConsoleOutputString(DEFINE_RESERVED_PARAMETERS);
		static void ExitProcess(DEFINE_RESERVED_PARAMETERS);
		static DWORD CreateProcess(DEFINE_RESERVED_PARAMETERS);
		static DWORD TerminateProcess(DEFINE_RESERVED_PARAMETERS);
		static DWORD OpenCurrentProcessHandle(DEFINE_RESERVED_PARAMETERS);
		static DWORD OpenProcessHandle(DEFINE_RESERVED_PARAMETERS);
		static DWORD CreateThread(DEFINE_RESERVED_PARAMETERS);
		static DWORD OpenThread(DEFINE_RESERVED_PARAMETERS);
		static DWORD PauseThread(DEFINE_RESERVED_PARAMETERS);
		static DWORD ResumeThread(DEFINE_RESERVED_PARAMETERS);
		static DWORD SetThreadPriority(DEFINE_RESERVED_PARAMETERS);
		static DWORD SetThreadStatus(DEFINE_RESERVED_PARAMETERS);
		static DWORD TerminateThread(DEFINE_RESERVED_PARAMETERS);
		static DWORD GetTid(DEFINE_RESERVED_PARAMETERS);
		static DWORD GetExitCode(DEFINE_RESERVED_PARAMETERS);
		static DWORD GetThreadPriority(DEFINE_RESERVED_PARAMETERS);
		static DWORD GetThreadStatus(DEFINE_RESERVED_PARAMETERS);
		static DWORD WaitForThreadExit(DEFINE_RESERVED_PARAMETERS);
		static void ExitThread(DEFINE_RESERVED_PARAMETERS);
		// Returns the tid of the current thread.
		static DWORD GetCurrentThreadTid(DEFINE_RESERVED_PARAMETERS);
		// Returns a new handle of the current thread.
		static HANDLE GetCurrentThreadHandle(DEFINE_RESERVED_PARAMETERS);
		// Returns the priority of the current thread.
		static multitasking::Thread::priority_t GetCurrentThreadPriority(DEFINE_RESERVED_PARAMETERS);
		// Returns the status of the current thread (most likely THREAD_RUNNING).
		static DWORD GetCurrentThreadStatus(DEFINE_RESERVED_PARAMETERS);
		static DWORD CloseHandle(DEFINE_RESERVED_PARAMETERS);
		static DWORD OpenProcessParentHandle(DEFINE_RESERVED_PARAMETERS);
		static DWORD SetConsoleColor(DEFINE_RESERVED_PARAMETERS);
		static DWORD GetConsoleColor(DEFINE_RESERVED_PARAMETERS);
		static DWORD ConsoleOutputCharacter(DEFINE_RESERVED_PARAMETERS);
		static DWORD ConsoleOutputCharacterAt(DEFINE_RESERVED_PARAMETERS);
		static DWORD ConsoleOutput(DEFINE_RESERVED_PARAMETERS);
		static DWORD ConsoleFillLine(DEFINE_RESERVED_PARAMETERS);
		static DWORD SetConsoleCursorPosition(DEFINE_RESERVED_PARAMETERS);
		static DWORD GetConsoleCursorPosition(DEFINE_RESERVED_PARAMETERS);
		static DWORD ConsoleSwapBuffers(DEFINE_RESERVED_PARAMETERS);
		static DWORD ClearConsole(DEFINE_RESERVED_PARAMETERS);
		static DWORD VirtualAlloc(DEFINE_RESERVED_PARAMETERS);
		static DWORD VirtualFree(DEFINE_RESERVED_PARAMETERS);
		static BOOL HasVirtualAddress(DEFINE_RESERVED_PARAMETERS);
		static DWORD MemoryProtect(DEFINE_RESERVED_PARAMETERS);

		UINTPTR_T g_syscallTable[512];
		void RegisterSyscalls()
		{
			utils::dwMemset(g_syscallTable, (DWORD)do_nothing, sizeof(g_syscallTable) / sizeof(g_syscallTable[0]));
			DWORD nextSyscall = 0;
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(ConsoleOutputString);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(ExitProcess);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(CreateProcess);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(TerminateProcess);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(OpenCurrentProcessHandle);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(OpenProcessHandle);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(CreateThread);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(OpenThread);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(PauseThread);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(ResumeThread);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(SetThreadPriority);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(SetThreadStatus);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(TerminateThread);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(GetTid);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(GetExitCode);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(GetThreadPriority);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(GetThreadStatus);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(WaitForThreadExit);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(ExitThread);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(GetCurrentThreadTid);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(GetCurrentThreadHandle);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(GetCurrentThreadPriority);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(GetCurrentThreadStatus);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(CloseHandle);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(OpenProcessParentHandle);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(SetConsoleColor);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::GetConsoleColor);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::GetConsoleColor);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::ConsoleOutputCharacter);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::ConsoleOutputCharacterAt);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::ConsoleOutput);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::ConsoleFillLine);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::SetConsoleCursorPosition);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::GetConsoleCursorPosition);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::ConsoleSwapBuffers);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::ClearConsole);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::VirtualAlloc);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::VirtualFree);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::HasVirtualAddress);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::MemoryProtect);
		}

		static void ConsoleOutputString(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return;
			struct _par
			{
				CSTRING string;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr((PVOID)par->string))
				return;
			obos::ConsoleOutputString(par->string);
		}
		static void ExitProcess(DEFINE_RESERVED_PARAMETERS)
		{
			if(!checkUserPtr(parameters))
				multitasking::g_currentThread->owner->TerminateProcess(0);
			struct _par
			{
				DWORD exitCode;
			} *par = reinterpret_cast<_par*>(parameters);
			multitasking::g_currentThread->owner->TerminateProcess(par->exitCode);
		}
		static DWORD CreateProcess(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				PBYTE elfFile; 
				SIZE_T size; 
				HANDLE* mainThread;
				HANDLE* procHandle; 
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr(par->procHandle))
				return 0xFFFFF000;
			Process* ret = new Process{};
			*par->procHandle = (HANDLE)ret;
			return ret->CreateProcess(par->elfFile, par->size, par->mainThread, false);
		}
		static DWORD TerminateProcess(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE procHandle;
				DWORD exitCode;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->procHandle))
				return 0xFFFFF000;
			Process* ret = reinterpret_cast<Process*>(par->procHandle);
			if (ret->pid >= g_nextProcessId)
				return 0xFFFFF000; // That isn't a process.
			if (ret->magicNumber != 0xCA44C071)
				return 0xFFFFF000; // This isn't a process.
			if (!ret->parent)
				return 0xFFFFF000; // It is extremely unlikely the kernel calls this.
			return ret->TerminateProcess(par->exitCode);
		}
		DWORD OpenCurrentProcessHandle(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE* procHandle;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr(par->procHandle))
				return 0xFFFFF000;
			*par->procHandle = (HANDLE)multitasking::g_currentThread->owner;
			return 0;
		}
		DWORD OpenProcessHandle(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE* procHandle;
				DWORD pid;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr(par->procHandle))
				return 0xFFFFF000;
			list_iterator_t* iter = list_iterator_new(process::g_processList, (par->pid > process::g_nextProcessId / 2) ? LIST_TAIL : LIST_HEAD);
			list_node_t* node = nullptr;
			for (node = list_iterator_next(iter); node; node = list_iterator_next(iter))
				if (static_cast<Process*>(node->val)->pid == par->pid)
					break;
			if (!node)
				return 1; // No such process.
			*par->procHandle = (HANDLE)node->val;
			return 0;
		}
		static DWORD CreateThread(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE* thrHandle;
				multitasking::Thread::priority_t threadPriority;
				VOID(*entry)(PVOID userData);
				PVOID userData;
				utils::RawBitfield threadStatus;
				SIZE_T stackSizePages;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr(par->thrHandle))
				return 0xFFFFF000;
			if (!checkUserPtr(reinterpret_cast<PVOID>(par->entry)))
				return 0xFFFFF000;
			multitasking::ThreadHandle* threadHandle = new multitasking::ThreadHandle{};
			*par->thrHandle = reinterpret_cast<HANDLE>(threadHandle);
			return threadHandle->CreateThread(par->threadPriority, par->entry, par->userData, par->threadStatus, par->stackSizePages);
		}
		static DWORD OpenThread(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE* thrHandle;
				DWORD tid;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr(par->thrHandle))
				return 0xFFFFF000;
			multitasking::ThreadHandle* threadHandle = new multitasking::ThreadHandle{};
			*par->thrHandle = reinterpret_cast<HANDLE>(threadHandle);
			list_iterator_t* iter = list_iterator_new(multitasking::g_threads, LIST_HEAD);
			list_node_t* node = list_iterator_next(iter);
			for (; node != nullptr; node = list_iterator_next(iter))
				if (((multitasking::Thread*)node->val)->tid == par->tid)
					break;
			if (!node)
				return 1; // The thread doesn't exist.
			bool ret = threadHandle->OpenThread((multitasking::Thread*)node->val);
			if (!ret)
				return 2; // Thread is dead.
			return 0;
		}
		static DWORD PauseThread(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE thrHandle;
				bool force;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->thrHandle))
				return 0xFFFFF000;
			Handle* handle = reinterpret_cast<Handle*>(par->thrHandle);
			if (handle->getType() != Handle::handleType::thread)
				return 0xFFFFF000;
			multitasking::ThreadHandle* threadHandle = static_cast<multitasking::ThreadHandle*>(handle);
			threadHandle->PauseThread(par->force);
			return 0;
		}
		static DWORD ResumeThread(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE thrHandle;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->thrHandle))
				return 0xFFFFF000;
			Handle* handle = reinterpret_cast<Handle*>(par->thrHandle);
			if (handle->getType() != Handle::handleType::thread)
				return 0xFFFFF000;
			multitasking::ThreadHandle* threadHandle = static_cast<multitasking::ThreadHandle*>(handle);
			threadHandle->ResumeThread();
			return 0;
		}
		static DWORD SetThreadPriority(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE thrHandle;
				multitasking::Thread::priority_t newPriority;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->thrHandle))
				return 0xFFFFF000;
			Handle* handle = reinterpret_cast<Handle*>(par->thrHandle);
			if (handle->getType() != Handle::handleType::thread)
				return 0xFFFFF000;
			multitasking::ThreadHandle* threadHandle = static_cast<multitasking::ThreadHandle*>(handle);
			threadHandle->SetThreadPriority(par->newPriority);
			return 0;
		}
		static DWORD SetThreadStatus(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE thrHandle;
				utils::RawBitfield newStatus;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->thrHandle))
				return 0xFFFFF000;
			Handle* handle = reinterpret_cast<Handle*>(par->thrHandle);
			if (handle->getType() != Handle::handleType::thread)
				return 0xFFFFF000;
			multitasking::ThreadHandle* threadHandle = static_cast<multitasking::ThreadHandle*>(handle);
			threadHandle->SetThreadStatus(par->newStatus);
			return 0;
		}
		static DWORD TerminateThread(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE thrHandle;
				DWORD exitCode;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->thrHandle))
				return 0xFFFFF000;
			if (*reinterpret_cast<HANDLE*>(par->thrHandle) == reinterpret_cast<HANDLE>(multitasking::g_currentThread))
				return 1;
			Handle* handle = reinterpret_cast<Handle*>(par->thrHandle);
			if (handle->getType() != Handle::handleType::thread)
				return 0xFFFFF000;
			multitasking::ThreadHandle* threadHandle = static_cast<multitasking::ThreadHandle*>(handle);
			threadHandle->TerminateThread(par->exitCode);
			return 0;
		}
		DWORD GetTid(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE thrHandle;
				DWORD* tid;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->thrHandle))
				return 0xFFFFF000;
			if (!checkUserPtr(par->tid))
				return 0xFFFFF000;
			if (*reinterpret_cast<HANDLE*>(par->thrHandle) == reinterpret_cast<HANDLE>(multitasking::g_currentThread))
			{
				*par->tid = multitasking::GetCurrentThreadTid();
				return 0;
			}
			Handle* handle = reinterpret_cast<Handle*>(par->thrHandle);
			if (handle->getType() != Handle::handleType::thread)
				return 0xFFFFF000;
			multitasking::ThreadHandle* threadHandle = static_cast<multitasking::ThreadHandle*>(handle);
			*par->tid = threadHandle->GetTid();
			return (*par->tid == static_cast<DWORD>(-1)) ? 1 : 0;
		}
		DWORD GetExitCode(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE thrHandle;
				DWORD* exitCode;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->thrHandle))
				return 0xFFFFF000;
			if (!checkUserPtr(par->exitCode))
				return 0xFFFFF000;
			if (*reinterpret_cast<HANDLE*>(par->thrHandle) == reinterpret_cast<HANDLE>(multitasking::g_currentThread))
				return 1;
			Handle* handle = reinterpret_cast<Handle*>(par->thrHandle);
			if (handle->getType() != Handle::handleType::thread)
				return 0xFFFFF000;
			multitasking::ThreadHandle* threadHandle = static_cast<multitasking::ThreadHandle*>(handle);
			*par->exitCode = threadHandle->GetExitCode();
			if (!(*par->exitCode))
			{
				auto status = threadHandle->GetThreadStatus();
				if (!status)
					return 2; // The thread wasn't created.
				if (status != static_cast<DWORD>(multitasking::Thread::status_t::DEAD))
					return 3; // The thread is running.
			}
			return 0;
		}
		DWORD GetThreadPriority(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE thrHandle;
				multitasking::Thread::priority_t* priority;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->thrHandle))
				return 0xFFFFF000;
			if (!checkUserPtr(par->priority))
				return 0xFFFFF000;
			if (*reinterpret_cast<HANDLE*>(par->thrHandle) == reinterpret_cast<HANDLE>(multitasking::g_currentThread))
			{
				*par->priority = multitasking::GetCurrentThreadPriority();
				return 0;
			}
			Handle* handle = reinterpret_cast<Handle*>(par->thrHandle);
			if (handle->getType() != Handle::handleType::thread)
				return 0xFFFFF000;
			multitasking::ThreadHandle* threadHandle = static_cast<multitasking::ThreadHandle*>(handle);
			*par->priority = threadHandle->GetThreadPriority();
			if (!par->priority)
				return 1; // The thread wasn't created.
			return 0;
		}
		DWORD GetThreadStatus(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE thrHandle;
				DWORD* status;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->thrHandle))
				return 0xFFFFF000;
			if (!checkUserPtr(par->status))
				return 0xFFFFF000;
			if (*reinterpret_cast<HANDLE*>(par->thrHandle) == reinterpret_cast<HANDLE>(multitasking::g_currentThread))
			{
				*par->status = multitasking::GetCurrentThreadStatus(); // Should always THREAD_RUNNING, but in case new statuses are added, keep this.
				return 0;
			}
			Handle* handle = reinterpret_cast<Handle*>(par->thrHandle);
			if (handle->getType() != Handle::handleType::thread)
				return 0xFFFFF000;
			multitasking::ThreadHandle* threadHandle = static_cast<multitasking::ThreadHandle*>(handle);
			*par->status = threadHandle->GetThreadStatus();
			if (!par->status)
				return 1; // The thread wasn't created.
			return 0;
		}
		static DWORD WaitForThreadExit(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE thrHandle;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->thrHandle))
				return 0xFFFFF000;
			if (*reinterpret_cast<HANDLE*>(par->thrHandle) == reinterpret_cast<HANDLE>(multitasking::g_currentThread))
				return 1; // Cannot wait for the current thread to die.
			Handle* handle = reinterpret_cast<Handle*>(par->thrHandle);
			if (handle->getType() != Handle::handleType::thread)
				return 0xFFFFF000;
			multitasking::ThreadHandle* threadHandle = static_cast<multitasking::ThreadHandle*>(handle);
			if (!threadHandle->WaitForThreadExit())
				return 2;
			return 0;
		}
		static void ExitThread(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				multitasking::ExitThread(0xFFFFF000);
			struct _par
			{
				DWORD exitCode;
			} *par = reinterpret_cast<_par*>(parameters);
			multitasking::ExitThread(par->exitCode);
		}
		static DWORD GetCurrentThreadTid(DEFINE_RESERVED_PARAMETERS)
		{
			return multitasking::GetCurrentThreadTid();
		}
		static HANDLE GetCurrentThreadHandle(DEFINE_RESERVED_PARAMETERS)
		{
			return reinterpret_cast<HANDLE>(multitasking::GetCurrentThreadHandle());
		}
		static multitasking::Thread::priority_t GetCurrentThreadPriority(DEFINE_RESERVED_PARAMETERS)
		{
			return multitasking::GetCurrentThreadPriority();
		}
		static DWORD GetCurrentThreadStatus(DEFINE_RESERVED_PARAMETERS)
		{
			return multitasking::GetCurrentThreadStatus();
		}
		static DWORD CloseHandle(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE handle;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->handle))
				return 0xFFFFF000;
			Handle* _handle = reinterpret_cast<Handle*>(par->handle);
			_handle->closeHandle();
			delete _handle;
			return 0;
		}
		static DWORD OpenProcessParentHandle(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				HANDLE procHandle;
				HANDLE* parentHandle;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->procHandle))
				return 0xFFFFF000;
			if (!checkUserPtr(par->parentHandle))
				return 0xFFFFF000;
			Process* proc = reinterpret_cast<Process*>(par->procHandle);
			if (proc->pid >= g_nextProcessId)
				return 0xFFFFF000; // That isn't a process.
			if (proc->magicNumber != 0xCA44C071)
				return 0xFFFFF000; // This isn't a process.
			if (!proc->parent)
				return 0xFFFFF000; // It is extremely unlikely the kernel calls this.
			*par->parentHandle = reinterpret_cast<HANDLE>(proc->parent);
			return 0;
		}
		static DWORD SetConsoleColor(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				DWORD fgColour;
				DWORD bgColour;
			} *par = reinterpret_cast<_par*>(parameters);
			obos::SetConsoleColor(par->fgColour, par->bgColour);
			return 0;
		}
		static DWORD GetConsoleColor(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				DWORD* fgColour;
				DWORD* bgColour;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr(par->fgColour))
				return 0xFFFFF000;
			if (!checkUserPtr(par->bgColour))
				return 0xFFFFF000;
			obos::GetConsoleColor(*par->fgColour, *par->bgColour);
			return 0;
		}
		static DWORD ConsoleOutputCharacter(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				CHAR ch;
				bool swapBuffers;
			} *par = reinterpret_cast<_par*>(parameters);
			obos::ConsoleOutputCharacter(par->ch, par->swapBuffers);
			return 0;
		}
		static DWORD ConsoleOutputCharacterAt(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				CHAR ch;
				DWORD x, y;
				bool swapBuffers;
			} *par = reinterpret_cast<_par*>(parameters);
			obos::ConsoleOutputCharacter(par->ch, par->x, par->y, par->swapBuffers);
			return 0;
		}
		static DWORD ConsoleOutput(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				CSTRING out;
				SIZE_T size;
				bool swapBuffers;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr(par->out))
				return 0xFFFFF000;
			obos::ConsoleOutput(par->out, par->size, par->swapBuffers);
			return 0;
		}
		static DWORD ConsoleFillLine(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				CHAR ch;
				bool swapBuffers;
			} *par = reinterpret_cast<_par*>(parameters);
			obos::ConsoleFillLine(par->ch, par->swapBuffers);
			return 0;
		}
		static DWORD SetConsoleCursorPosition(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				DWORD x, y;
			} *par = reinterpret_cast<_par*>(parameters);
			obos::SetTerminalCursorPosition(par->x, par->y);
			return 0;
		}
		static DWORD GetConsoleCursorPosition(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				DWORD *x, *y;
			} *par = reinterpret_cast<_par*>(parameters);
			obos::GetTerminalCursorPosition(*par->x, *par->y);
			return 0;
		}
		static DWORD ConsoleSwapBuffers(DEFINE_RESERVED_PARAMETERS)
		{
			obos::swapBuffers();
			return 0;
		}
		static DWORD ClearConsole(DEFINE_RESERVED_PARAMETERS)
		{
			obos::ClearConsole();
			return 0;
		}
		static DWORD VirtualAlloc(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				PVOID _base;
				SIZE_T nPages; 
				utils::RawBitfield flags;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr(par->_base, true) && par->_base != nullptr)
				return 0xFFFFF000;
			return (DWORD)memory::VirtualAlloc(par->_base, par->nPages, par->flags | memory::VirtualAllocFlags::GLOBAL);
		}
		static DWORD VirtualFree(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				PVOID _base;
				SIZE_T nPages;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr(par->_base))
				return 0xFFFFF000;
			return memory::VirtualFree(par->_base, par->nPages);
		}
		static BOOL HasVirtualAddress(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				PCVOID _base;
				SIZE_T nPages;
			} *par = reinterpret_cast<_par*>(parameters);
			if (par->nPages == 1)
				return checkUserPtr(par->_base);
			return memory::HasVirtualAddress(par->_base, par->nPages) && checkUserPtr(par->_base, false);
		}
		static DWORD MemoryProtect(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
				return 0xFFFFF000;
			struct _par
			{
				PVOID _base;
				SIZE_T nPages; 
				utils::RawBitfield flags;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr(par->_base))
				return 0xFFFFF000;
			return memory::MemoryProtect(par->_base, par->nPages, par->flags | memory::VirtualAllocFlags::GLOBAL);
		}
	}
}