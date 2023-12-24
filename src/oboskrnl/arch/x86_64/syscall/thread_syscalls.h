/*
	arch/x86_64/syscall/memory_syscalls.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace syscalls
	{

		// bool CreateThread(uintptr_t _this, uint32_t priority, size_t stackSize, uint64_t affinity, void(*entry)(uintptr_t), uintptr_t userdata, bool startPaused);
		// bool OpenThread(uintptr_t _this, uint32_t tid);
		// bool PauseThread(uintptr_t _this);
		// bool ResumeThread(uintptr_t _this);
		// bool SetThreadPriority(uintptr_t _this, uint32_t priority);
		// bool TerminateThread(uintptr_t _this, uint32_t exitCode);
		// 
		// uint32_t GetThreadStatus(uintptr_t _this);
		// uint32_t GetThreadExitCode(uintptr_t _this);
		// uint32_t GetThreadLastError(uintptr_t _this);
		// 
		// bool CloseThread(uintptr_t _this);
		// bool FreeThread(uintptr_t _this);
		
		uintptr_t SyscallMakeThreadObject();

		bool SyscallCreateThread(void* pars);
		bool SyscallOpenThread(void* pars);
		bool SyscallPauseThread(uintptr_t* pars);
		bool SyscallResumeThread(uintptr_t* pars);
		bool SyscallSetThreadPriority(void* pars);
		bool SyscallTerminateThread(void* pars);
		
		uint32_t SyscallGetThreadStatus(uintptr_t* _this);
		uint32_t SyscallGetThreadExitCode(uintptr_t* _this);
		uint32_t SyscallGetThreadLastError(uintptr_t* _this);
		
		bool SyscallCloseThread(uintptr_t* _this);
		void SyscallFreeThread(uintptr_t* _this);
	}
}