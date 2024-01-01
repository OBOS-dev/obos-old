/*
	oboskrnl/allocators/vmm/arch.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

#include <multitasking/process/process.h>

namespace obos
{
	namespace memory
	{
		enum VAllocStatus
		{
			VALLOC_SUCCESS,
			VALLOC_INVALID_PARAMETER,
			VALLOC_BASE_ADDRESS_USED,
			VALLOC_ACCESS_DENIED,
		};
		enum VProtectStatus
		{
			VPROTECT_SUCCESS,
			VPROTECT_INVALID_PARAMETER,
			VPROTECT_MEMORY_UNMAPPED,
			VPROTECT_ACCESS_DENIED,
		};
		enum VGetProtStatus
		{
			VGETPROT_SUCCESS,
			VGETPROT_INVALID_PARAMETER,
			VGETPROT_ACCESS_DENIED,
		};
		enum VFreeStatus
		{
			VFREE_SUCCESS,
			VFREE_INVALID_PARAMETER,
			VFREE_MEMORY_UNMAPPED,
			VFREE_ACCESS_DENIED,
		};
		enum MemcpyStatus
		{
			MEMCPY_SUCCESS,
			MEMCPY_DESTINATION_PFAULT,
			MEMCPY_SOURCE_PFAULT,
		};
		/// <summary>
		/// Finds a free region of nPages size as "proc."
		/// </summary>
		/// <param name="proc">The process.</param>
		/// <param name="nPages">The minimum amount of pages the free region must be.</param>
		/// <returns>The address, or nullptr if there is no such address.</returns>
		OBOS_EXPORT void* _Impl_FindUsableAddress(process::Process* proc, size_t nPages);
		/// <summary>
		/// Maps "nPages" pages with the flags "protFlags" at base as the process "proc." If this function fails, it must undo any changes to the address.
		/// </summary>
		/// <param name="proc">The process to map as.</param>
		/// <param name="base">The base address.</param>
		/// <param name="nPages">The amount of pages to map.</param>
		/// <param name="protFlags">The protection flags.</param>
		/// <param name="status">(out) The function's status (enum VAllocStatus)</param>
		/// <returns>The base address, or nullptr on failure.</returns>
		void* _Impl_ProcVirtualAlloc(process::Process* proc, void* base, size_t nPages, uintptr_t protFlags, uint32_t* status);
		/// <summary>
		/// Unmaps "nPages" at "base" as "proc."
		/// </summary>
		/// <param name="proc">The process to unmap as.</param>
		/// <param name="base">The base of where to unmap. This cannot be nullptr.</param>
		/// <param name="nPages">The amount of pages to unmap.</param>
		/// <param name="status">(out) The function's status (enum VFreeStatus)</param>
		void _Impl_ProcVirtualFree(process::Process* proc, void* base, size_t nPages, uint32_t* status);
		/// <summary>
		/// Changes the protection flags at "base" for nPages as "proc."
		/// </summary>
		/// <param name="proc">The process to change the protection flags as.</param>
		/// <param name="base">The base address.</param>
		/// <param name="nPages">The amount of pages after and including "base" to change the protection flags.</param>
		/// <param name="flags">The new flags.</param>
		/// <param name="status">(out) The function's status (enum VProtectStatus)</param>
		void _Impl_ProcVirtualProtect(process::Process* proc, void* base, size_t nPages, uintptr_t flags, uint32_t* status);
		/// <summary>
		/// Gets the protection flags for "base" for "nPages" as "proc."
		/// </summary>
		/// <param name="proc">The process to change the protection flags as.</param>
		/// <param name="base">The base address.</param>
		/// <param name="nPages">The amount of pages after and including "base" to retrieve the protection flags.</param>
		/// <param name="flags">(out) A pointer to an array of size nPages * sizeof(uintptr_t) to store the flags in.</param>
		/// <param name="status">(out) The function's status (enum VGetProtStatus)</param>
		void _Impl_ProcVirtualGetProtection(process::Process* proc, void* base, size_t nPages, uintptr_t* flags, uint32_t* status);

		/// <summary>
		/// Checks if the address is valid.
		/// This can, for example on x86-64, check if the address is canonical.
		/// </summary>
		/// <param name="addr">The address to check.</param>
		/// <returns>Whether the check failed (false) or not (true).</returns>
		OBOS_EXPORT bool _Impl_IsValidAddress(void* addr);
		/// <summary>
		/// Gets the page size for the architecture.
		/// </summary>
		/// <returns>The page size.</returns>
		size_t _Impl_GetPageSize();

		/// <summary>
		/// Copies size bytes of localSrc in the current context to remoteDest in proc's context.
		/// </summary>
		/// <param name="proc">The process.</param>
		/// <param name="remoteDest">A memory address in the process's context to copy to (must not be PROT_READ_ONLY).</param>
		/// <param name="localSrc">The local address to copy from.</param>
		/// <param name="size">The amount of bytes to copy.</param>
		/// <param name="status">(out) The function's status (enum MemcpyStatus)</param>
		/// <returns>remoteDest on success, otherwise nullptr.</returns>
		void* _Impl_Memcpy(process::Process* proc, void* remoteDest, void* localSrc, size_t size, uint32_t* status);

		/// <summary>
		/// Frees the user process's address space.
		/// </summary>
		/// <param name="proc">The process. This cannot be a wildcard (nullptr).</param>
		/// <returns>Whether the function succedeed (true) or not (false).</returns>
		bool _Impl_FreeUserProcessAddressSpace(process::Process* proc);
	}
}