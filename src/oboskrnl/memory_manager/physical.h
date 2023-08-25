/*
	oboskrnl/memory_manager/physical.h

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
		// Take the memory address of this for the intended value.
		extern char __etext;
		// Take the memory address of this for the intended value.
		extern char __erodata;
		// Take the memory address of this for the intended value.
		extern char endImage;
		// Take the memory address of this for the intended value.
		extern char endKernel;

		bool InitializePhysicalMemoryManager();

		PVOID kalloc_physicalPage();
		INT   kfree_physicalPage(PVOID base);

		UINT32_T* kmap_physical(PVOID _base, SIZE_T nPages, utils::RawBitfield flags, PVOID physicalAddress, bool force = false);
	}
}