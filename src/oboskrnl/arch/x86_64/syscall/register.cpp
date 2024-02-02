/*
	arch/x86_64/syscall/register.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <error.h>

#include <arch/x86_64/syscall/register.h>
#include <arch/x86_64/syscall/thread.h>
#include <arch/x86_64/syscall/handle.h>
#include <arch/x86_64/syscall/console.h>
#include <arch/x86_64/syscall/vmm.h>
#include <arch/x86_64/syscall/signals.h>

#include <arch/x86_64/syscall/verify_pars.h>

#include <arch/x86_64/syscall/vfs/file.h>
#include <arch/x86_64/syscall/vfs/disk.h>

#include <driverInterface/load.h>
#include <driverInterface/struct.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t ErrorSyscallHandler(uint64_t syscall, void* args)
		{
			switch (syscall)
			{
			case 55:
				return GetLastError();
			case 56:
			{
				struct _pars
				{
					alignas(0x10) uint32_t errorCode;
				} *pars = (_pars*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
					return 0;
				SetLastError(pars->errorCode);
				break;
			}
			default:
				break;
			}
			return 0;
		}
		bool LoadModuleSyscallHandler(uint64_t, void* args)
		{
			struct _pars
			{
				alignas(0x10) const byte* data;
				alignas(0x10) size_t size;
			} *pars = (_pars*)args;
			if (!canAccessUserMemory(pars, sizeof(*pars), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!canAccessUserMemory(pars->data, pars->size, false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			// End early if the user passed an invalid module.
			// If we don't end early and the user passed an invalid module, we would waste a lot of heap space.
			auto header = driverInterface::CheckModule(pars->data, pars->size);
			if (!header || header->magicNumber != driverInterface::OBOS_DRIVER_HEADER_MAGIC)
			{
				if (header->magicNumber != driverInterface::OBOS_DRIVER_HEADER_MAGIC)
					SetLastError(OBOS_ERROR_INVALID_DRIVER_HEADER);
				return false;
			}
			// Copy the data into a kernel buffer as apparently LoadModule casts away const somewhere and writes to the data parameter.
			// It's expensive, but at least we won't break our promises made to userspace (const byte* as opposed to byte* in the syscall parameters.)
			byte* kData = (byte*)utils::memcpy(new char[pars->size], pars->data, pars->size);
			bool ret = driverInterface::LoadModule(kData, pars->size, nullptr);
			delete[] kData;
			return ret;
		}
		uintptr_t g_syscallTable[g_syscallTableLimit + 1];
		void RegisterSyscalls()
		{
			for (uint16_t currentSyscall = 0; currentSyscall < 13; RegisterSyscall(currentSyscall++, (uintptr_t)ThreadSyscallHandler));
			for (uint16_t currentSyscall = 13; currentSyscall < 23; RegisterSyscall(currentSyscall++, (uintptr_t)FileHandleSyscallHandler));
			RegisterSyscall(23, (uintptr_t)ThreadSyscallHandler);
			RegisterSyscall(24, (uintptr_t)SyscallInvalidateHandle);
			for (uint16_t currentSyscall = 25; currentSyscall < 38; RegisterSyscall(currentSyscall++, (uintptr_t)ConsoleSyscallHandler));
			for (uint16_t currentSyscall = 39; currentSyscall < 45; RegisterSyscall(currentSyscall++, (uintptr_t)VMMSyscallHandler));
			for (uint16_t currentSyscall = 46; currentSyscall < 55; RegisterSyscall(currentSyscall++, (uintptr_t)DriveSyscallHandler));
			for (uint16_t currentSyscall = 55; currentSyscall < 57; RegisterSyscall(currentSyscall++, (uintptr_t)ErrorSyscallHandler));
			RegisterSyscall(57, (uintptr_t)LoadModuleSyscallHandler);
			for (uint16_t currentSyscall = 57; currentSyscall < 59; RegisterSyscall(currentSyscall++, (uintptr_t)SignalsSyscallHandler));

		}
		void RegisterSyscall(uint16_t n, uintptr_t func)
		{
			if (n > g_syscallTableLimit)
				logger::warning("Hit the syscall limit. Overflowed by %d.\n", n - g_syscallTableLimit);
			g_syscallTable[n] = func;
		}
	}
}