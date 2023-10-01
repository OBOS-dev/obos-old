/*
	oboskrnl/syscalls/syscalls.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <syscalls/syscalls.h>

#include <utils/memory.h>

#include <types.h>
#include <console.h>
#include <error.h>

#include <multitasking/multitasking.h>
#include <multitasking/thread.h>
#include <multitasking/threadHandle.h>

#include <process/process.h>

#include <memory_manager/paging/allocate.h>

extern int do_nothing();

#define DEFINE_RESERVED_PARAMETERS PVOID parameters

#if defined(__x86_64__)
#define STACK_SIZE 8
#elif defined(__i686__)
#define STACK_SIZE 4
#endif

namespace obos
{
	extern char kernelStart;
}

static bool checkUserPtr(PCVOID ptr, bool isAllocCall = false)
{
	UINTPTR_T iPtr = reinterpret_cast<UINTPTR_T>(ptr);
	
	if (iPtr >= obos::process::ProcEntryPointBase && iPtr < (obos::process::ProcEntryPointBase + 0x1000))
		return false;

#ifndef __x86_64__
#ifdef __i686__
	if (iPtr < 0x401000)
		return false;
	if (iPtr >= 0xC0000000 && iPtr < 0xE0000000)
		return false;
	if (iPtr >= 0xFFCFF000)
		return false;
#endif
	if (!obos::memory::HasVirtualAddress(ptr, 1))
		return isAllocCall;
#else
	if (iPtr >= (UINTPTR_T)&obos::kernelStart)
		return false;
	if (!obos::memory::HasVirtualAddress(ptr, 1, false))
		return isAllocCall;
#endif
	return true;
}
static bool checkUserHandle(HANDLE handle)
{
	obos::Handle* _handle = (obos::Handle*)handle;
#ifdef __x86_64
	if (!obos::memory::HasVirtualAddress(_handle, 1, false))
		return false;
	return handle >= 0xfffffffff0000000 && _handle->magicNumber == OBOS_HANDLE_MAGIC_NUMBER;
#elif defined(__i686__)
	if(!obos::memory::HasVirtualAddress(_handle, 1))
		return false;
	return (handle >= 0xC0000000 && handle < 0xE0000000) && _handle->magicNumber == OBOS_HANDLE_MAGIC_NUMBER;
#endif
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
		static DWORD GetCurrentThreadTid();
		// Returns a new handle of the current thread.
		static HANDLE GetCurrentThreadHandle();
		// Returns the priority of the current thread.
		static multitasking::Thread::priority_t GetCurrentThreadPriority();
		// Returns the status of the current thread (most likely THREAD_RUNNING).
		static DWORD GetCurrentThreadStatus();
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
		static DWORD ConsoleSwapBuffers();
		static DWORD ClearConsole();
		static PVOID VirtualAlloc(DEFINE_RESERVED_PARAMETERS);
		static DWORD VirtualFree(DEFINE_RESERVED_PARAMETERS);
		static BOOL HasVirtualAddress(DEFINE_RESERVED_PARAMETERS);
		static DWORD MemoryProtect(DEFINE_RESERVED_PARAMETERS);
		static DWORD GetLastError();
		static DWORD SetLastError(DEFINE_RESERVED_PARAMETERS);

		UINTPTR_T g_syscallTable[512];
		void RegisterSyscalls()
		{
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
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::SetConsoleColor);
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
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::GetLastError);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(process::SetLastError);
#if defined(__i686__)
			utils::dwMemset(g_syscallTable + nextSyscall, (DWORD)do_nothing, sizeof(g_syscallTable) / sizeof(g_syscallTable[0]));
#elif defined(__x86_64__)
			for (SIZE_T i = nextSyscall; i < sizeof(g_syscallTable) / sizeof(g_syscallTable[0]); i++)
				g_syscallTable[i] = reinterpret_cast<UINTPTR_T>(do_nothing);
#endif
		}

		static void ConsoleOutputString(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters = parameters))
				return;
			struct _par
			{
				alignas(STACK_SIZE) CSTRING string;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
			if (!checkUserPtr((PVOID)par->string))
				return;
			obos::ConsoleOutputString(par->string);
		}
		static void ExitProcess(DEFINE_RESERVED_PARAMETERS)
		{
			if(!checkUserPtr(parameters = parameters))
				multitasking::g_currentThread->owner->TerminateProcess(0);
			struct _par
			{
				alignas(STACK_SIZE) DWORD exitCode;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
			multitasking::g_currentThread->owner->TerminateProcess(par->exitCode);
		}
		static DWORD CreateProcess(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) PBYTE elfFile; 
				alignas(STACK_SIZE) SIZE_T size; 
				alignas(STACK_SIZE) HANDLE* mainThread;
				alignas(STACK_SIZE) HANDLE* procHandle; 
			} *par = reinterpret_cast<_par*>(parameters = parameters);
			if (!checkUserPtr(par->procHandle))
				return 0xFFFFF000;
			Process* ret = new Process{};
			*par->procHandle = (HANDLE)ret;
			return ret->CreateProcess(par->elfFile, par->size, par->mainThread, false);
		}
		static DWORD TerminateProcess(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE procHandle;
				alignas(STACK_SIZE) DWORD exitCode;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
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
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE* procHandle;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
			if (!checkUserPtr(par->procHandle))
				return 0xFFFFF000;
			*par->procHandle = (HANDLE)multitasking::g_currentThread->owner;
			return 0;
		}
		DWORD OpenProcessHandle(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE* procHandle;
				alignas(STACK_SIZE) DWORD pid;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
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
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE* thrHandle;
				alignas(STACK_SIZE) multitasking::Thread::priority_t threadPriority;
				alignas(STACK_SIZE) VOID(*entry)(PVOID userData);
				alignas(STACK_SIZE) PVOID userData;
				alignas(STACK_SIZE) utils::RawBitfield threadStatus;
				alignas(STACK_SIZE) SIZE_T stackSizePages;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
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
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE* thrHandle;
				alignas(STACK_SIZE) DWORD tid;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
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
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE thrHandle;
				alignas(STACK_SIZE) bool force;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
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
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE thrHandle;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
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
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE thrHandle;
				alignas(STACK_SIZE) multitasking::Thread::priority_t newPriority;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
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
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE thrHandle;
				alignas(STACK_SIZE) utils::RawBitfield newStatus;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
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
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE thrHandle;
				alignas(STACK_SIZE) DWORD exitCode;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
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
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE thrHandle;
				alignas(STACK_SIZE) DWORD* tid;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
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
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE thrHandle;
				alignas(STACK_SIZE) DWORD* exitCode;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
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
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE thrHandle;
				alignas(STACK_SIZE) multitasking::Thread::priority_t* priority;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
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
			if (!checkUserPtr(parameters = parameters))
				return 0xFFFFF000;
			struct _par
			{
				alignas(STACK_SIZE) HANDLE thrHandle;
				alignas(STACK_SIZE) DWORD* status;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
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
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) HANDLE thrHandle;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->thrHandle))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			if (*reinterpret_cast<HANDLE*>(par->thrHandle) == reinterpret_cast<HANDLE>(multitasking::g_currentThread))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			} // Cannot wait for the current thread to die.
			Handle* handle = reinterpret_cast<Handle*>(par->thrHandle);
			if (handle->getType() != Handle::handleType::thread)
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			multitasking::ThreadHandle* threadHandle = static_cast<multitasking::ThreadHandle*>(handle);
			return threadHandle->WaitForThreadExit();
		}
		static void ExitThread(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters = parameters))
				multitasking::ExitThread(0);
			struct _par
			{
				alignas(STACK_SIZE) DWORD exitCode;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
			multitasking::ExitThread(par->exitCode);
		}
		static DWORD GetCurrentThreadTid()
		{
			return multitasking::GetCurrentThreadTid();
		}
		static HANDLE GetCurrentThreadHandle()
		{
			return reinterpret_cast<HANDLE>(multitasking::GetCurrentThreadHandle());
		}
		static multitasking::Thread::priority_t GetCurrentThreadPriority()
		{
			return multitasking::GetCurrentThreadPriority();
		}
		static DWORD GetCurrentThreadStatus()
		{
			return multitasking::GetCurrentThreadStatus();
		}
		static DWORD CloseHandle(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) HANDLE handle;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->handle))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			Handle* _handle = reinterpret_cast<Handle*>(par->handle);
			_handle->closeHandle();
			delete _handle;
			return 0;
		}
		static DWORD OpenProcessParentHandle(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) HANDLE procHandle;
				alignas(STACK_SIZE) HANDLE* parentHandle;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserHandle(par->procHandle))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			if (!checkUserPtr(par->parentHandle))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			Process* proc = reinterpret_cast<Process*>(par->procHandle);
			if (proc->pid >= g_nextProcessId)
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER; // That isn't a process.
			}
			if (proc->magicNumber != 0xCA44C071)
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER; // This isn't a process.
			}
			if (!proc->parent)
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER; // It is extremely unlikely the kernel calls this.
			}
			*par->parentHandle = reinterpret_cast<HANDLE>(proc->parent);
			return 0;
		}
		static DWORD SetConsoleColor(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) DWORD fgColour;
				alignas(STACK_SIZE) DWORD bgColour;
			} *par = reinterpret_cast<_par*>(parameters);
			obos::SetConsoleColor(par->fgColour, par->bgColour);
			return 0;
		}
		static DWORD GetConsoleColor(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) DWORD* fgColour;
				alignas(STACK_SIZE) DWORD* bgColour;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr(par->fgColour))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			if (!checkUserPtr(par->bgColour))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			obos::GetConsoleColor(*par->fgColour, *par->bgColour);
			return 0;
		}
		static DWORD ConsoleOutputCharacter(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) CHAR ch;
				alignas(STACK_SIZE) bool swapBuffers;
			} *par = reinterpret_cast<_par*>(parameters);
			obos::ConsoleOutputCharacter(par->ch, par->swapBuffers);
			return 0;
		}
		static DWORD ConsoleOutputCharacterAt(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) CHAR ch;
				alignas(STACK_SIZE) DWORD x, y;
				alignas(STACK_SIZE) bool swapBuffers;
			} *par = reinterpret_cast<_par*>(parameters);
			obos::ConsoleOutputCharacter(par->ch, par->x, par->y, par->swapBuffers);
			return 0;
		}
		static DWORD ConsoleOutput(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) CSTRING out;
				alignas(STACK_SIZE) SIZE_T size;
				alignas(STACK_SIZE) bool swapBuffers;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr(par->out))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			obos::ConsoleOutput(par->out, par->size, par->swapBuffers);
			return 0;
		}
		static DWORD ConsoleFillLine(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) CHAR ch;
				alignas(STACK_SIZE) bool swapBuffers;
			} *par = reinterpret_cast<_par*>(parameters);
			obos::ConsoleFillLine(par->ch, par->swapBuffers);
			return 0;
		}
		static DWORD SetConsoleCursorPosition(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) DWORD x, y;
			} *par = reinterpret_cast<_par*>(parameters);
			obos::SetTerminalCursorPosition(par->x, par->y);
			return 0;
		}
		static DWORD GetConsoleCursorPosition(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) DWORD *x, *y;
			} *par = reinterpret_cast<_par*>(parameters);
			obos::GetTerminalCursorPosition(*par->x, *par->y);
			return 0;
		}
		static DWORD ConsoleSwapBuffers()
		{
			obos::swapBuffers();
			return 0;
		}
		static DWORD ClearConsole()
		{
			obos::ClearConsole();
			return 0;
		}
		static PVOID VirtualAlloc(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return nullptr;
			}
			struct _par
			{
				alignas(STACK_SIZE) PVOID _base;
				alignas(STACK_SIZE) SIZE_T nPages; 
				alignas(STACK_SIZE) utils::RawBitfield flags;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
			if (!checkUserPtr(par->_base, true) && par->_base != nullptr)
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return nullptr;
			}
			return memory::VirtualAlloc(par->_base, par->nPages, par->flags | memory::VirtualAllocFlags::GLOBAL);
		}
		static DWORD VirtualFree(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) PVOID _base;
				alignas(STACK_SIZE) SIZE_T nPages;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
			if (!checkUserPtr(par->_base))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			return memory::VirtualFree(par->_base, par->nPages);
		}
		static BOOL HasVirtualAddress(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			};
			struct _par
			{
				alignas(STACK_SIZE) PCVOID _base;
				alignas(STACK_SIZE) SIZE_T nPages;
			} *par = reinterpret_cast<_par*>(parameters = parameters);
			if (par->nPages == 1)
				return checkUserPtr(par->_base);
			return memory::HasVirtualAddress(par->_base, par->nPages) && checkUserPtr(par->_base, false);
		}
		static DWORD MemoryProtect(DEFINE_RESERVED_PARAMETERS)
		{
			if (!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) PVOID _base;
				alignas(STACK_SIZE) SIZE_T nPages; 
				alignas(STACK_SIZE) utils::RawBitfield flags;
			} *par = reinterpret_cast<_par*>(parameters);
			if (!checkUserPtr(par->_base))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			return memory::MemoryProtect(par->_base, par->nPages, par->flags | memory::VirtualAllocFlags::GLOBAL);
		}
		DWORD GetLastError()
		{
			return obos::GetLastError();
		}
		DWORD SetLastError(DEFINE_RESERVED_PARAMETERS)
		{
			if(!checkUserPtr(parameters))
			{
				obos::SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return OBOS_ERROR_INVALID_PARAMETER;
			}
			struct _par
			{
				alignas(STACK_SIZE) DWORD newErrorCode;
			} *par = reinterpret_cast<_par*>(parameters);
			obos::SetLastError(par->newErrorCode);
			return 0;
		}
	}
}