/*
	allocate.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>
#include <utils/bitfields.h>

#define ROUND_ADDRESS_DOWN(addr) ((UINTPTR_T)((addr >> 12) << 12))
#define ROUND_ADDRESS_UP(addr) ((UINTPTR_T)((addr >> 12) << 12) + 4096)

namespace obos
{
	namespace memory
	{
		/// <summary>
		/// The flags for VirtualAlloc();
		/// </summary>
		class VirtualAllocFlags
		{
		public:
			enum
			{
				WRITE_ENABLED = 2,
				GLOBAL = 4,
			};
			VirtualAllocFlags() = default;
			VirtualAllocFlags(UINT32_T value) 
				:m_val{value}
			{}

			UINT32_T& getVal() { return m_val; }
			operator UINT32_T() { return m_val; }
		private:
			UINT32_T m_val;
		};
		/// <summary>
		/// Allocates virtual memory.
		/// </summary>
		/// <param name="base">The base address. If this is less than 0x400000, the function will choose a base address.</param>
		/// <param name="nPages">The amount of pages to allocate.</param>
		/// <param name="flags">The allocation flags.</param>
		/// <returns>The base address. If the base address was already allocated, nullptr.</returns>
		PVOID VirtualAlloc(PVOID base, SIZE_T nPages, utils::RawBitfield flags);
		/// <summary>
		/// Frees virtual memory. This will clear the pages if it doesn't fail.
		/// </summary>
		/// <param name="base">The base address. The function will fail if the base address is less than 0x400000.</param>
		/// <param name="nPages">The amount of pages to free.</param>
		/// <returns>One if the pages weren't allocated or the address is less than 0x400000, otherwise zero.</returns>
		DWORD VirtualFree(PVOID base, SIZE_T nPages);
		/// <summary>
		/// Checks if the base was allocated by the program.
		/// </summary>
		/// <param name="base">The base address of the pages.</param>
		/// <param name="nPages">The amount of pages to check.</param>
		/// <returns>false if the pages were never allocated, otherwise true.</returns>
		bool HasVirtualAddress(PCVOID base, SIZE_T nPages);
	}
}