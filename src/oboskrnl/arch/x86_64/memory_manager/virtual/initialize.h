/*
	arch/x86_64/memory_manager/virtual/initialize.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

namespace obos
{
	namespace memory
	{
		OBOS_EXPORT uintptr_t* mapPageTable(uintptr_t* phys);

		class PageMap
		{
		public:
			PageMap() = delete;

			// L4 -> Page Map
			// L3 -> Page Directory Pointer
			// L2 -> Page Directory
			// L1 -> Page Table
			// L0 -> Page Table Entry

			OBOS_EXPORT uintptr_t* getPageMap() { return (uintptr_t*)this; }
			OBOS_EXPORT uintptr_t* getL4PageMapEntryAt(uintptr_t at); // pageMap[addressToIndex(at, 3)];
			OBOS_EXPORT uintptr_t* getL3PageMapEntryAt(uintptr_t at); // getL4PageMapEntryAt()[addressToIndex(at,2)];
			OBOS_EXPORT uintptr_t* getL2PageMapEntryAt(uintptr_t at); // getL3PageMapEntryAt()[addressToIndex(at,1)];
			OBOS_EXPORT uintptr_t* getL1PageMapEntryAt(uintptr_t at); // getL2PageMapEntryAt()[addressToIndex(at,0)];

			OBOS_EXPORT void switchToThis();

			OBOS_EXPORT static size_t addressToIndex(uintptr_t address, uint8_t level) { return (address >> (9 * level + 12)) & 0x1FF; }
		};

		OBOS_EXPORT PageMap* getCurrentPageMap();

		OBOS_EXPORT size_t GetPhysicalAddressBits();
		OBOS_EXPORT size_t GetVirtualAddressBits();

		void InitializeVirtualMemoryManager();

		OBOS_EXPORT bool CPUSupportsExecuteDisable();
		
		// We don't export this because by the time a driver gets loaded it will be true.

		extern bool g_initialized;

		extern OBOS_EXPORT uintptr_t g_physAddrMask;
	}
}