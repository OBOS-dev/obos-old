/*
	arch/x86_64/syscall/vmm.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <error.h>
#include <memory_manipulation.h>

#include <arch/x86_64/syscall/handle.h>
#include <arch/x86_64/syscall/verify_pars.h>
#include <arch/x86_64/syscall/vmm.h>

#include <allocators/vmm/vmm.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t VMMSyscallHandler(uint64_t syscall, void* args)
		{
			switch (syscall)
			{
			case 39:
			{
				struct _par
				{
					alignas(0x10) process::Process* owner;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return UINTPTR_MAX;
				}
				return SyscallCreateVirtualAllocator(pars->owner);
			}
			case 40:
			{
				struct _par
				{
					alignas(0x10) uintptr_t hnd;
					alignas(0x10) void* base;
					alignas(0x10) size_t size;
					alignas(0x10) uintptr_t flags;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return UINTPTR_MAX;
				}
				return (uintptr_t)SyscallVirtualAlloc(pars->hnd, pars->base, pars->size, pars->flags);
			}
			case 41:
			{
				struct _par
				{
					alignas(0x10) uintptr_t hnd;
					alignas(0x10) void* base;
					alignas(0x10) size_t size;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return UINTPTR_MAX;
				}
				return SyscallVirtualFree(pars->hnd, pars->base, pars->size);
			}
			case 42:
			{
				struct _par
				{
					alignas(0x10) uintptr_t hnd;
					alignas(0x10) void* base;
					alignas(0x10) size_t size;
					alignas(0x10) uintptr_t flags;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return UINTPTR_MAX;
				}
				return SyscallVirtualProtect(pars->hnd, pars->base, pars->size, pars->flags);
			}
			case 43:
			{
				struct _par
				{
					alignas(0x10) uintptr_t hnd;
					alignas(0x10) void* base;
					alignas(0x10) size_t size;
					alignas(0x10) uintptr_t* flags;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return UINTPTR_MAX;
				}
				return SyscallVirtualGetProtection(pars->hnd, pars->base, pars->size, pars->flags);
			}
			case 44:
			{
				struct _par
				{
					alignas(0x10) uintptr_t hnd;
					alignas(0x10) void* dest;
					alignas(0x10) const void* src;
					alignas(0x10) size_t size;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return UINTPTR_MAX;
				}
				return (uintptr_t)SyscallVirtualMemcpy(pars->hnd, pars->dest, pars->src, pars->size);
			}
			default:
				break;
			}
			return 0;
		}

		user_handle SyscallCreateVirtualAllocator(process::Process* owner)
		{
			if (owner != nullptr)
			{
				if ((uintptr_t)owner < 0xffff'ffff'f000'0000)
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return UINTPTR_MAX;
				}
				if (!owner->isUsermode)
				{
					SetLastError(OBOS_ERROR_INVALID_PARAMETER);
					return UINTPTR_MAX;
				}
			}
			return ProcessRegisterHandle(nullptr, new memory::VirtualAllocator{ owner }, ProcessHandleType::VALLOCATOR_HANDLE);
		}

		// TODO: Make this configurable.
		constexpr size_t g_noDemandPagingSzLimit = 0x400000 /* 4 MiB */;
		void* SyscallVirtualAlloc(user_handle hnd, void* base, size_t size, uintptr_t flags)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::VALLOCATOR_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return nullptr;
			}
			if ((uintptr_t)base > 0xffff'8000'0000'0000)
			{
				// No.
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				return nullptr;
			}
			flags &= memory::PROT_ALL_BITS_SET;
			if (size > g_noDemandPagingSzLimit && flags & memory::PROT_NO_COW_ON_ALLOCATE)
			{
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				return nullptr;
			}
			flags |= memory::PROT_USER_MODE_ACCESS;
			memory::VirtualAllocator* valloc = (memory::VirtualAllocator*)ProcessGetHandleObject(nullptr, hnd);
			return valloc->VirtualAlloc(base, size, flags);
		}
		bool SyscallVirtualFree(user_handle hnd, void* base, size_t size)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::VALLOCATOR_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if ((uintptr_t)base > 0xffff'8000'0000'0000)
			{
				// No.
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				return false;
			}
			memory::VirtualAllocator* valloc = (memory::VirtualAllocator*)ProcessGetHandleObject(nullptr, hnd);
			return valloc->VirtualFree(base, size);
		}
		bool SyscallVirtualProtect(user_handle hnd, void* base, size_t size, uintptr_t flags)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::VALLOCATOR_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if ((uintptr_t)base > 0xffff'8000'0000'0000)
			{
				// No.
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				return false;
			}
			flags |= memory::PROT_USER_MODE_ACCESS;
			memory::VirtualAllocator* valloc = (memory::VirtualAllocator*)ProcessGetHandleObject(nullptr, hnd);
			return valloc->VirtualProtect(base, size, flags);
		}
		bool SyscallVirtualGetProtection(user_handle hnd, void* base, size_t size, uintptr_t* flags)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::VALLOCATOR_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			size_t pageSize = memory::VirtualAllocator::GetPageSize();
			const size_t nPages = size / pageSize + ((size % pageSize) != 0);
			if (!canAccessUserMemory(flags, nPages * sizeof(*flags), true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return false;
			}
			if ((uintptr_t)base > 0xffff'8000'0000'0000)
			{
				// No.
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				return false;
			}
			if (!flags)
				return true;
			memory::VirtualAllocator* valloc = (memory::VirtualAllocator*)ProcessGetHandleObject(nullptr, hnd);
			uintptr_t* _flags = new uintptr_t[nPages];
			bool ret = valloc->VirtualGetProtection(base, size, _flags);
			if(ret)
				utils::memcpy(flags, _flags, nPages * sizeof(*flags));
			return ret;
		}

		void* SyscallVirtualMemcpy(user_handle hnd, void* remoteDest, const void* localSrc, size_t size)
		{
			if (!ProcessVerifyHandle(nullptr, hnd, ProcessHandleType::VALLOCATOR_HANDLE))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return nullptr;
			}
			if ((uintptr_t)remoteDest > 0xffff'8000'0000'0000 || (uintptr_t)localSrc > 0xffff'8000'0000'0000)
			{
				// No.
				SetLastError(OBOS_ERROR_ACCESS_DENIED);
				return nullptr;
			}
			memory::VirtualAllocator* valloc = (memory::VirtualAllocator*)ProcessGetHandleObject(nullptr, hnd);
			return valloc->Memcpy(remoteDest, localSrc, size);
		}
	}
}