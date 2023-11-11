/*
	arch/x86_64/syscall/thread_syscalls.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <error.h>

#include <arch/x86_64/syscall/thread_syscalls.h>
#include <arch/x86_64/syscall/verify_pars.h>

#include <multitasking/threadAPI/thrHandle.h>
#include <multitasking/scheduler.h>

#include <multitasking/process/process.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t SyscallMakeThreadObject()
		{
			return (uintptr_t)new thread::ThreadHandle;
		}

		bool SyscallCreateThread(void* pars)
		{
			struct _par
			{
				alignas(0x08) thread::ThreadHandle* _this; 
				alignas(0x08) uint32_t priority; 
				alignas(0x08) size_t stackSize; 
				alignas(0x08) void(*entry)(uintptr_t);
				alignas(0x08) uintptr_t userdata;
				alignas(0x08) bool startPaused;
			} *par = (_par*)pars;
			if (!canAccessUserMemory(par, sizeof(*par), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!canAccessUserMemory(par->_this, sizeof(thread::ThreadHandle), true, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if ((uintptr_t)par->entry > 0xffffffff80000000)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			return par->_this->CreateThread(par->priority, par->stackSize, par->entry, par->userdata, nullptr, par->startPaused, ((process::Process*)thread::g_currentThread->owner)->isUsermode);
		}
		bool SyscallOpenThread(void* pars)
		{
			struct _par
			{
				alignas(0x08) thread::ThreadHandle* _this;
				alignas(0x08) uint32_t tid;
			} *par = (_par*)pars;
			if (!canAccessUserMemory(par, sizeof(*par), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!canAccessUserMemory(par->_this, sizeof(thread::ThreadHandle), true, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (par->tid < 2 || par->tid > thread::g_nextTid)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			return par->_this->OpenThread(par->tid);
		}
		bool SyscallPauseThread(uintptr_t* pars)
		{
			if (!canAccessUserMemory(pars, sizeof(*pars), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!canAccessUserMemory((void*)*pars, sizeof(thread::ThreadHandle), true, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			thread::ThreadHandle* _this = (thread::ThreadHandle*)*pars;
			return _this->PauseThread();
		}
		bool SyscallResumeThread(uintptr_t* pars)
		{
			if (!canAccessUserMemory(pars, sizeof(*pars), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!canAccessUserMemory((void*)*pars, sizeof(thread::ThreadHandle), true, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			thread::ThreadHandle* _this = (thread::ThreadHandle*)*pars;
			return _this->ResumeThread();
		}
		bool SyscallSetThreadPriority(void* pars)
		{
			struct _par
			{
				alignas(0x08) thread::ThreadHandle* _this;
				alignas(0x08) uint32_t priority;
			} *par = (_par*)pars;
			if (!canAccessUserMemory(par, sizeof(*par), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!canAccessUserMemory(par->_this, sizeof(thread::ThreadHandle), true, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			return par->_this->SetThreadPriority(par->priority);
		}
		bool SyscallTerminateThread(void* pars)
		{
			struct _par
			{
				alignas(0x08) thread::ThreadHandle* _this;
				alignas(0x08) uint32_t exitCode;
			} *par = (_par*)pars;
			if (!canAccessUserMemory(par, sizeof(*par), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!canAccessUserMemory(par->_this, sizeof(thread::ThreadHandle), true, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			return par->_this->TerminateThread(par->exitCode);
		}

		uint32_t SyscallGetThreadStatus(uintptr_t* _this)
		{
			if (!canAccessUserMemory(_this, sizeof(*_this), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return 0;
			}
			if (!canAccessUserMemory((void*)*_this, sizeof(thread::ThreadHandle), true, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return 0;
			}
			thread::ThreadHandle* this_ = (thread::ThreadHandle*)*_this;
			return this_->GetThreadStatus();
		}
		uint32_t SyscallGetThreadExitCode(uintptr_t* _this)
		{
			if (!canAccessUserMemory(_this, sizeof(*_this), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return 0;
			}
			if (!canAccessUserMemory((void*)*_this, sizeof(thread::ThreadHandle), true, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return 0;
			}
			thread::ThreadHandle* this_ = (thread::ThreadHandle*)*_this;
			return this_->GetThreadExitCode();
		}
		uint32_t SyscallGetThreadLastError(uintptr_t* _this)
		{
			if (!canAccessUserMemory(_this, sizeof(*_this), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return 0;
			}
			if (!canAccessUserMemory((void*)*_this, sizeof(thread::ThreadHandle), true, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return 0;
			}
			thread::ThreadHandle* this_ = (thread::ThreadHandle*)*_this;
			return this_->GetThreadLastError();
		}

		bool SyscallCloseThread(uintptr_t* _this)
		{
			if (!canAccessUserMemory(_this, sizeof(*_this), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return 0;
			}
			if (!canAccessUserMemory((void*)*_this, sizeof(thread::ThreadHandle), true, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return 0;
			}
			thread::ThreadHandle* this_ = (thread::ThreadHandle*)*_this;
			return this_->CloseHandle();
		}
		void SyscallFreeThread(uintptr_t* _this)
		{
			if (!canAccessUserMemory(_this, sizeof(*_this), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (!canAccessUserMemory((void*)*_this, sizeof(thread::ThreadHandle), true, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			thread::ThreadHandle* this_ = (thread::ThreadHandle*)*_this;
			delete this_;
		}
	}
}