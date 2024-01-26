/*
	arch/x86_64/syscall/vmm.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <arch/x86_64/syscall/handle.h>

#include <multitasking/process/process.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t VMMSyscallHandler(uint64_t syscall, void* args);

		/// <summary>
		/// Syscall Number: 39<para></para>
		/// Creates a virtual allocator with 'owner' as the process to allocate as.
		/// </summary>
		/// <param name="owner">The process to allocate as.</param>
		user_handle SyscallCreateVirtualAllocator(process::Process* owner);

		/// <summary>
		/// Syscall Number: 40<para></para>
		/// Allocates nPages at base.
		/// </summary>
		/// <param name="base">The base address to allocate at.</param>
		/// <param name="size">The amount of bytes (rounded to the nearest page size) to allocate at base.</param>
		/// <param name="flags">The initial protection flags.</param>
		/// <returns>"base" if base isn't nullptr. If base is nullptr, the function finds a base address. </returns>
		void* SyscallVirtualAlloc(user_handle hnd, void* base, size_t size, uintptr_t flags);
		/// <summary>
		/// Syscall Number: 41<para></para>
		/// Free nPages pages at base.
		/// </summary>
		/// <param name="base">The base address to free.</param>
		/// <param name="size">The amount of bytes (rounded to the nearest page size) to free.</param>
		/// <returns>false on failure, otherwise true. If this function fails, use GetLastError for extra error information.</returns>
		bool SyscallVirtualFree(user_handle hnd, void* base, size_t size);
		/// <summary>
		/// Syscall Number: 42<para></para>
		/// Sets the protection for the pages at base.
		/// </summary>
		/// <param name="base">The base address to set the protection for.</param>
		/// <param name="size">The amount of bytes (rounded to the nearest page size) to set the protection for.</param>
		/// <param name="flags">The new protection flags</param>
		/// <returns>false on failure, otherwise true. If this function fails, use GetLastError for extra error information.</returns>
		bool SyscallVirtualProtect(user_handle hnd, void* base, size_t size, uintptr_t flags);
		/// <summary>
		/// Syscall Number: 43<para></para>
		/// Gets the protection for base.
		/// </summary>
		/// <param name="base">The base address to get the protection for.</param>
		/// <param name="size">The amount of bytes (rounded to the nearest page size) to get the protection for.</param>
		/// <param name="flags">A pointer to a buffer of the size "sizeof(PageProtectionFlags) * sizeToPageCount(size)" to store the protection in.</param>
		/// <returns>false on failure, otherwise true. If this function fails, use GetLastError for extra error information.</returns>
		bool SyscallVirtualGetProtection(user_handle hnd, void* base, size_t size, uintptr_t* flags);

		/// <summary>
		/// Syscall Number: 44<para></para>
		/// Copies a buffer from this program to the other process.
		/// </summary>
		/// <param name="remoteDest">The remote destination's address.</param>
		/// <param name="localSrc">The local source's address.</param>
		/// <param name="size">The size of the buffer.</param>
		/// <returns>remoteDest on success, or nullptr.</returns>
		void* SyscallVirtualMemcpy(user_handle hnd, void* remoteDest, const void* localSrc, size_t size);
	}
}