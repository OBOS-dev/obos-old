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
		// Don't modify unless you're the physical memory manager.
		extern utils::Bitfield g_availablePages[g_countPages / 32];

		bool InitializePhysicalMemoryManager();

		PVOID kalloc_physicalPages(SIZE_T nPages);
		PVOID kfree_physicalPage(SIZE_T nPages);
	}
}