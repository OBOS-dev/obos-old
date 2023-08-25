/*
	oboskrnl/memory_manager/x86-64/physical.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/physical.h>

#include <types.h>

#include <boot/boot.h>

#define inRange(val, rStart, rEnd) (((UINTPTR_T)(val)) >= ((UINTPTR_T)(rStart)) && val <= ((UINTPTR_T)(rEnd)))

namespace obos
{
	extern void RestartComputer();

	namespace memory
	{
		UINTPTR_T* kmap_pageTable(PVOID physicalAddress);

		UINT64_T* g_physicalMemoryBitfield = nullptr;
		SIZE_T g_sizeOfMemory = 0;
		SIZE_T g_nPagesAllocated = 0;
		SIZE_T g_bitfieldCount = 0;
		SIZE_T g_mmapLength = 0;

		static SIZE_T s_availablePagesIndex = 0;
		static SIZE_T s_availablePagesBit = 0;

		static SIZE_T addressToIndex(UINTPTR_T address, SIZE_T* bit)
		{
			if (bit)
				*bit = (address >> 12) % 32;
			return address >> 17;
		}
		static UINTPTR_T indexToAddress(SIZE_T index, SIZE_T bit)
		{
			return ((index << 5) << 12) + (bit << 12);
		}
		static bool isAddressUsed(UINTPTR_T address)
		{
			multiboot_memory_map_t* current = reinterpret_cast<multiboot_memory_map_t*>(g_multibootInfo->mmap_addr);
			for (SIZE_T i = 0; i < g_mmapLength; i++)
			{
				if (current->type != MULTIBOOT_MEMORY_AVAILABLE && current->addr >= (UINTPTR_T)&endImage)
					if (inRange(address, current->addr & 0xFFF, ((current->addr + current->len) + 0xFFF) & 0xFFF))
						return false;
			}
			if (inRange(address, reinterpret_cast<UINTPTR_T>(g_physicalMemoryBitfield) & 0xFFF, reinterpret_cast<UINTPTR_T>(g_physicalMemoryBitfield + g_bitfieldCount) & 0xFFF))
				return false;
			return true;
		}

		bool InitializePhysicalMemoryManager()
		{
			// Calculate the size of memory.
			multiboot_memory_map_t* mmap = reinterpret_cast<multiboot_memory_map_t*>(g_multibootInfo->mmap_addr);
			g_mmapLength = g_multibootInfo->mmap_length / mmap[0].size;
			for (SIZE_T i = 0; i < g_mmapLength; i++)
				g_sizeOfMemory += mmap[i].len;
			g_bitfieldCount = g_sizeOfMemory >> 20;
			// Find a place to put g_physicalMemoryBitfield at.
			for (SIZE_T i = 0; i < g_mmapLength && g_physicalMemoryBitfield; i++)
				if (mmap[i].len >= (g_bitfieldCount * 8) && mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE)
					g_physicalMemoryBitfield = reinterpret_cast<UINT64_T*>(mmap[i].addr);
			if (!g_physicalMemoryBitfield)
				RestartComputer(); // Oh no!
			return true;
		}

		PVOID kalloc_physicalPage()
		{
			constexpr SIZE_T maxTries = 8;
			for (SIZE_T i = 0; i < maxTries; i++)
			{
				for (; s_availablePagesIndex < g_bitfieldCount; s_availablePagesIndex++)
				{
					UINT64_T* physicalMemoryBitfield = (UINT64_T*)kmap_pageTable(g_physicalMemoryBitfield + s_availablePagesIndex);
					if (*physicalMemoryBitfield == 0xFFFFFFFF)
						continue;
					for (; s_availablePagesBit < 32; s_availablePagesBit++)
					{
						if (!utils::testBitInBitfield(*physicalMemoryBitfield, s_availablePagesBit))
							break;
					}
					if (!utils::testBitInBitfield(*physicalMemoryBitfield, s_availablePagesBit))
						break;
				}
				UINT64_T* physicalMemoryBitfield = (UINT64_T*)kmap_pageTable(g_physicalMemoryBitfield + s_availablePagesIndex);
				UINTPTR_T address = indexToAddress(s_availablePagesIndex, s_availablePagesBit);
				if (isAddressUsed(address))
					continue;
				utils::setBitInBitfield(*physicalMemoryBitfield, s_availablePagesBit);
				return reinterpret_cast<PVOID>(address);
			}
			return nullptr;
		}
		INT kfree_physicalPage(PVOID _base)
		{
			UINTPTR_T base = reinterpret_cast<UINTPTR_T>(_base);
			SIZE_T bit = 0;
			SIZE_T index = addressToIndex(base, &bit);
			if (index > g_bitfieldCount)
				return 1;
			UINT64_T* physicalMemoryBitfield = (UINT64_T*)kmap_pageTable(g_physicalMemoryBitfield + s_availablePagesIndex);
			if (!utils::testBitInBitfield(*physicalMemoryBitfield, bit))
				return 1;
			s_availablePagesIndex = index;
			s_availablePagesBit = bit;
			utils::clearBitInBitfield(*physicalMemoryBitfield, bit);
			return 0;
		}
	}
}