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
		/// <summary>
		/// Allocates nPages at base.
		/// </summary>
		/// <param name="base">The base address to allocate at.</param>
		/// <param name="nPages">The amount of pages to allocate at base.</param>
		/// <param name="flags">The initial protection flags.</param>
		/// <returns>"base" if base isn't nullptr. If base is nullptr, the function finds a base address. On failure, this function returns nullptr. Use GetLastError to check the error.</returns>
		void* SyscallVirtualAlloc(void* pars);
		/// <summary>
		/// Free nPages pages at base.
		/// </summary>
		/// <param name="base">The base address to free.</param>
		/// <param name="nPages">The amount of pages to free.</param>
		/// <returns>false on failure, otherwise true. If this function fails, use GetLastError for extra error information.</returns>
		bool SyscallVirtualFree(void* pars);
		/// <summary>
		/// Sets the protection for the pages at base.
		/// </summary>
		/// <param name="base">The base address to set the protection for.</param>
		/// <param name="nPages">The amount of pages to set the protection for.</param>
		/// <param name="flags">The new protection flags</param>
		/// <returns>false on failure, otherwise true. If this function fails, use GetLastError for extra error information.</returns>
		bool SyscallVirtualProtect(void* pars);
		/// <summary>
		/// Gets the protection for base.
		/// </summary>
		/// <param name="base">The base address to get the protection for.</param>
		/// <param name="nPages">The amount of pages to get the protection for.</param>
		/// <param name="flags">A pointer to a buffer of the size "sizeof(PageProtectionFlags) * nPages" to store the protection in.</param>
		/// <returns>false on failure, otherwise true. If this function fails, use GetLastError for extra error information.</returns>
		bool SyscallVirtualGetProtection(void* pars);
	}
}