/*
	arch/x86_64/memory_manager/virtual/allocate.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace memory
	{
		enum PageProtectionFlags
		{
			/// <summary>
			/// Whether the pages allocated can be read.
			/// </summary>
			PROT_READ_ONLY = 0b1,
			/// <summary>
			/// Whether user mode threads (cs=0x1b,ds=0x23) can access the pages allocated. This is the default when using any syscalls.
			/// </summary>
			PROT_USER_MODE_ACCESS = 0b10,
			/// <summary>
			/// Whether the pages can be executed or not. If the processor doesn't support this, all pages are executable
			/// </summary>
			PROT_CAN_EXECUTE = 0b100,
			/// <summary>
			/// Whether to allocate the pages immediately, or to COW (Copy on write) the allocated pages.
			/// </summary>
			PROT_NO_COW_ON_ALLOCATE = 0b1000,
			/// <summary>
			/// Whether to disable cache on the allocated pages.
			/// </summary>
			PROT_DISABLE_CACHE = 0b10000,
			/// <summary>
			/// Whether the pages are present. This is ignored in VirtualAlloc and VirtualProtect
			/// </summary>
			PROT_IS_PRESENT = 0b100000,
			PROT_ALL_BITS_SET = 0b111111,
		};

		/// <summary>
		/// Decodes PageProtectionFlags into a format that the CPU understands.
		/// </summary>
		/// <param name="_flags">The PageProtectionFlags</param>
		/// <returns>The decoded flags.</returns>
		uintptr_t DecodeProtectionFlags(uintptr_t _flags);

		/// <summary>
		/// Allocates nPages at base.
		/// </summary>
		/// <param name="base">The base address to allocate at.</param>
		/// <param name="nPages">The amount of pages to allocate at base.</param>
		/// <param name="flags">The initial protection flags.</param>
		/// <returns>"base" if base isn't nullptr. If base is nullptr, the function finds a base address. </returns>
		void* VirtualAlloc(void* base, size_t nPages, uintptr_t flags);
		/// <summary>
		/// Free nPages pages at base.
		/// </summary>
		/// <param name="base">The base address to free.</param>
		/// <param name="nPages">The amount of pages to free.</param>
		/// <returns>false on failure, otherwise true. If this function fails, use GetLastError for extra error information.</returns>
		bool VirtualFree(void* base, size_t nPages);
		/// <summary>
		/// Sets the protection for the pages at base.
		/// </summary>
		/// <param name="base">The base address to set the protection for.</param>
		/// <param name="nPages">The amount of pages to set the protection for.</param>
		/// <param name="flags">The new protection flags</param>
		/// <returns>false on failure, otherwise true. If this function fails, use GetLastError for extra error information.</returns>
		bool VirtualProtect(void* base, size_t nPages, uintptr_t flags);
		/// <summary>
		/// Gets the protection for base.
		/// </summary>
		/// <param name="base">The base address to get the protection for.</param>
		/// <param name="nPages">The amount of pages to get the protection for.</param>
		/// <param name="flags">A pointer to a buffer of the size "sizeof(PageProtectionFlags) * nPages" to store the protection in.</param>
		/// <returns>false on failure, otherwise true. If this function fails, use GetLastError for extra error information.</returns>
		bool VirtualGetProtection(void* base, size_t nPages, uintptr_t* flags);

		void MapVirtualPageToPhysical(void* virt, void* phys, uintptr_t cpuFlags);
		void MapVirtualPageToEntry(void* virt, uintptr_t entry);
	}
}