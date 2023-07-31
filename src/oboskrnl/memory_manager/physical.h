/*
	physical.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#include <utils/bitfields.h>

namespace obos
{
	namespace memory
	{
		constexpr SIZE_T g_countPages = 1048576;
		constexpr SIZE_T g_oneMib = 0x100000;
		// Don't modify unless you're the physical memory manager.
		extern utils::RawBitfield g_availablePages[g_countPages / 32];
		// Take the memory address of this for the intended value.
		extern char __etext;
		// Take the memory address of this for the intended value.
		extern char __erodata;
		// Take the memory address of this for the intended value.
		extern char endImage;
		// Take the memory address of this for the intended value.
		extern char endKernel;

		bool InitializePhysicalMemoryManager();

		PVOID kalloc_physicalPages(SIZE_T nPages);
		INT   kfree_physicalPages(PVOID base, SIZE_T nPages);
	}
}