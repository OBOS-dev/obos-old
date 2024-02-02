/*
	arch/x86_64/syscall/signals.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <error.h>

#include <multitasking/process/process.h>
#include <multitasking/process/signals.h>

#include <arch/x86_64/syscall/handle.h>
#include <arch/x86_64/syscall/signals.h>

#include <arch/x86_64/syscall/verify_pars.h>

#include <multitasking/cpu_local.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t SignalsSyscallHandler(uint64_t syscall, void* args)
		{
			switch (syscall)
			{
			case 57:
			{
				struct _par
				{
					alignas(0x10) process::signals signal;
					alignas(0x10) uintptr_t uhandler;
				} *par = (_par*)args;
				if (!canAccessUserMemory(par, sizeof(*par), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				return SyscallRegisterSignal(par->signal, par->uhandler);
			}
			case 58:
			{
				struct _par
				{
					alignas(0x10) user_handle on;
					alignas(0x10) process::signals sig;
				} *par = (_par*)args;
				if (!canAccessUserMemory(par, sizeof(*par), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return false;
				}
				return SyscallCallSignal(par->on, par->sig);
			}
			default:
				break;
			}
			return 0;
		}

		bool SyscallRegisterSignal(process::signals signal, uintptr_t uhandler)
		{
			if ((uint32_t)signal > (uint32_t)process::signals::INVALID_SIGNAL)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			process::Process* currentProc = (process::Process*)thread::GetCurrentCpuLocalPtr()->currentThread->owner;
			currentProc->signal_table[signal] = (void(*)())uhandler;
			return true;
		}
		bool SyscallCallSignal(user_handle on, process::signals sig)
		{
			if (!ProcessVerifyHandle(nullptr, on, ProcessHandleType::THREAD_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			return process::CallSignal((thread::Thread*)ProcessGetHandleObject(nullptr, on), sig);
		}
	}
}