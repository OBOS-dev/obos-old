/*
	arch/x86_64/syscall/memory_syscalls.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <error.h>

#include <arch/x86_64/memory_manager/virtual/allocate.h>

#include <arch/x86_64/syscall/verify_pars.h>
#include <arch/x86_64/syscall/memory_syscalls.h>

namespace obos
{
	namespace syscalls
	{
		void* SyscallVirtualAlloc(void* _pars)
		{
			struct _par
			{
				alignas(0x08) void* base;
				alignas(0x08) size_t nPages;
				alignas(0x08) uintptr_t flags;
			} *pars = (_par*)_pars;
			if (!canAccessUserMemory(pars, sizeof(_par), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return nullptr;
			}
			return memory::VirtualAlloc(pars->base, pars->nPages, (pars->flags | memory::PROT_USER_MODE_ACCESS) & ~memory::PROT_NO_COW_ON_ALLOCATE);
		}
		bool SyscallVirtualFree(void* _pars)
		{
			struct _par
			{
				alignas(0x08) void* base;
				alignas(0x08) size_t nPages;
			} *pars = (_par*)_pars;
			if (!canAccessUserMemory(pars, sizeof(_par), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			return memory::VirtualFree(pars->base, pars->nPages);
		}
		bool SyscallVirtualProtect(void* _pars)
		{
			struct _par
			{
				alignas(0x08) void* base;
				alignas(0x08) size_t nPages;
				alignas(0x08) uintptr_t flags;
			} *pars = (_par*)_pars;
			if (!canAccessUserMemory(pars, sizeof(_par), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			return memory::VirtualProtect(pars->base, pars->nPages, pars->flags | memory::PROT_USER_MODE_ACCESS);
		}
		bool SyscallVirtualGetProtection(void* _pars)
		{
			struct _par
			{
				alignas(0x08) void* base;
				alignas(0x08) size_t nPages;
				alignas(0x08) uintptr_t* flags;
			} *pars = (_par*)_pars;
			if (!canAccessUserMemory(pars, sizeof(_par), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if (!canAccessUserMemory(pars->flags, sizeof(_par), true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			return memory::VirtualGetProtection(pars->base, pars->nPages, pars->flags);
		}
	}
}