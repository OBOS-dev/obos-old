/*
	arch/x86_64/syscall/thread.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <error.h>

#include <multitasking/cpu_local.h>

#include <multitasking/threadAPI/thrHandle.h>

#include <arch/x86_64/syscall/handle.h>
#include <arch/x86_64/syscall/thread.h>
#include <arch/x86_64/syscall/verify_pars.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t ThreadSyscallHandler(uint64_t syscall, void* args)
		{
			bool (*bool_handler)(user_handle) = nullptr;
			bool (*bool_p32_handler)(user_handle, uint32_t) = nullptr;
			uint32_t (*u32_handler)(user_handle) = nullptr;
			switch (syscall)
			{
			case 0:
				return SyscallMakeThreadHandle();
			case 2:
			{
				struct _pars
				{
					alignas(0x10) user_handle hnd; 
					alignas(0x10) uint32_t priority;
					alignas(0x10) size_t stackSize; 
					alignas(0x10) void(*entry)(uintptr_t); 
					alignas(0x10) uintptr_t userdata;
					alignas(0x10) uint64_t affinity;
					alignas(0x10) void* process;
					alignas(0x10) bool startPaused;
				} *pars = (_pars*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				return SyscallCreateThread(pars->hnd, pars->priority, pars->stackSize, pars->entry, pars->userdata, pars->affinity, pars->process, pars->startPaused);
			}
			case 4:
				bool_handler = SyscallResumeThread;
				goto _bool_handler;
			case 23:
				bool_handler = SyscallThreadCloseHandle;
				goto _bool_handler;
			case 3:
				bool_handler = SyscallPauseThread;
			{
			_bool_handler:
				struct _pars
				{
					alignas(0x10) user_handle hnd; 
				} *pars = (_pars*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				return bool_handler(pars->hnd);
			}
			case 1:
				bool_p32_handler = SyscallOpenThread;
				goto _bool_p32_handler;
			case 5:
				bool_p32_handler = SyscallSetThreadPriority;
				goto _bool_p32_handler;
			case 6:
				bool_p32_handler = SyscallTerminateThread;
			{
			_bool_p32_handler:
				struct _pars
				{
					alignas(0x10) user_handle hnd; 
					alignas(0x10) uint32_t par1;
				} *pars = (_pars*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				return bool_p32_handler(pars->hnd, pars->par1);
			}
			case 7:
				u32_handler = SyscallGetThreadStatus;
				goto _u32_handler;
			case 8:
				u32_handler = SyscallGetThreadExitCode;
				goto _u32_handler;
			case 9:
				u32_handler = SyscallGetThreadLastError;
				goto _u32_handler;
			case 10:
				u32_handler = SyscallGetThreadTID;
			{
			_u32_handler:
				struct _pars
				{
					alignas(0x10) user_handle hnd;
				} *pars = (_pars*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return 0xffffffff;
				}
				return u32_handler(pars->hnd);
			}
			case 11:
				return GetCurrentTID();
			case 12:
			{
				struct _pars
				{
					alignas(0x10) uint32_t exitCode;
				} *pars = (_pars*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
					ExitThread(OBOS_ERROR_INVALID_PARAMETER);
				ExitThread(pars->exitCode);
			}
			}
			logger::panic(nullptr, "Invalid syscall number for %s. Syscall number: %d", __func__, syscall);
		}

		user_handle SyscallMakeThreadHandle() 
		{
			return ProcessRegisterHandle(nullptr, new thread::ThreadHandle, ProcessHandleType::THREAD_HANDLE);
		}
		
		bool SyscallOpenThread(user_handle hnd, uint32_t tid)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::THREAD_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			thread::ThreadHandle* handle = (thread::ThreadHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->OpenThread(tid);
		}

		bool SyscallCreateThread(user_handle hnd, uint32_t priority, size_t stackSize, void(*entry)(uintptr_t), uintptr_t userdata, uint64_t affinity, void* process, bool startPaused)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::THREAD_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			thread::ThreadHandle* handle = (thread::ThreadHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->CreateThread(
				priority,
				stackSize,
				entry,
				userdata,
				!affinity ? thread::g_defaultAffinity : affinity,
				process,
				startPaused);
		}

		bool SyscallPauseThread(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::THREAD_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			thread::ThreadHandle* handle = (thread::ThreadHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->PauseThread();
		}
		bool SyscallResumeThread(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::THREAD_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			thread::ThreadHandle* handle = (thread::ThreadHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->ResumeThread();
		}
		bool SyscallSetThreadPriority(user_handle hnd, uint32_t priority)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::THREAD_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			thread::ThreadHandle* handle = (thread::ThreadHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->SetThreadPriority(priority);
		}
		bool SyscallTerminateThread(user_handle hnd, uint32_t exitCode)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::THREAD_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			thread::ThreadHandle* handle = (thread::ThreadHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->TerminateThread(exitCode);
		}

		uint32_t SyscallGetThreadStatus(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::THREAD_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			thread::ThreadHandle* handle = (thread::ThreadHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->GetThreadStatus();
		}
		uint32_t SyscallGetThreadExitCode(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::THREAD_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			thread::ThreadHandle* handle = (thread::ThreadHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->GetThreadExitCode();
		}
		uint32_t SyscallGetThreadLastError(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::THREAD_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			thread::ThreadHandle* handle = (thread::ThreadHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->GetThreadLastError();
		}
		uint32_t SyscallGetThreadTID(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::THREAD_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			thread::ThreadHandle* handle = (thread::ThreadHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->GetThreadTID();
		}

		bool SyscallThreadCloseHandle(user_handle hnd)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::THREAD_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			thread::ThreadHandle* handle = (thread::ThreadHandle*)ProcessGetHandleObject(nullptr, hnd);
			return handle->CloseHandle();
		}

		uint32_t GetCurrentTID()
		{
			return thread::GetCurrentCpuLocalPtr()->currentThread->tid;
		}

		[[noreturn]] void ExitThread(uint32_t exitCode)
		{
			::obos::thread::ExitThread(exitCode);
		}
	}
}