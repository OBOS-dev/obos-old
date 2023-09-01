/*
	oboskrnl/memory_manager/x86-64/physical.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/physical.h>

#include <types.h>

#include <boot/boot.h>

#include <utils/memory.h>

#define inRange(val, rStart, rEnd) (((UINTPTR_T)(val)) >= ((UINTPTR_T)(rStart)) && val <= ((UINTPTR_T)(rEnd)))

namespace obos
{
	extern void RestartComputer();

	namespace memory
	{
		UINTPTR_T* kmap_pageTable(PVOID physicalAddress);

		UINT32_T* g_physicalMemoryBitfield = nullptr;
		SIZE_T g_sizeOfMemory = 0;
		SIZE_T g_nPagesAllocated = 0;
		SIZE_T g_bitfieldCount = 0;
		SIZE_T g_mmapLength = 0;

		static SIZE_T addressToIndex(UINTPTR_T address, SIZE_T* bit)
		{
			if (bit)
				*bit = (address >> 12) % 32;
			return address >> 17;
		}
		/*static UINTPTR_T indexToAddress(SIZE_T index, SIZE_T bit)
		{
			return ((index << 5) << 12) + (bit << 12);
		}*/
		static bool isAddressUsed(UINTPTR_T address)
		{
			multiboot_memory_map_t* current = reinterpret_cast<multiboot_memory_map_t*>(g_multibootInfo->mmap_addr);
			for (SIZE_T i = 0; i < g_mmapLength; i++, current++)
			{
				if (current->type != MULTIBOOT_MEMORY_AVAILABLE)
					if (inRange(address, current->addr & (~0xFFF), ((current->addr + current->len) + 0xFFF) & (~0xFFF)))
						return true;
			}
			if ((address >= (reinterpret_cast<UINTPTR_T>(g_physicalMemoryBitfield) & (~0xFFF))) && address < ((reinterpret_cast<UINTPTR_T>(g_physicalMemoryBitfield + g_bitfieldCount) + 0xFFF) & (~0xFFF)))
				return true;
			if (inRange(address, 0x100000, 0x600000))
				return true;
			UINTPTR_T mmap_addr = g_multibootInfo->mmap_addr - 0xFFFFFFFF80000000;
			if (inRange(address, mmap_addr & (~0xFFF), ((mmap_addr + (g_mmapLength * sizeof(multiboot_memory_map_t)) + 0xFFF) & (~0xFFF))))
				return true;
			return false;
		}

		bool InitializePhysicalMemoryManager()
		{
			// Calculate the size of memory.
			multiboot_memory_map_t* mmap = reinterpret_cast<multiboot_memory_map_t*>(g_multibootInfo->mmap_addr);
			g_mmapLength = g_multibootInfo->mmap_length / mmap[0].size;
			for (SIZE_T i = 0; i < g_mmapLength; i++)
			{
				if (!mmap[i].type)
				{
					g_mmapLength = i;
					break;
				}
			}
			for (SIZE_T i = 0; i < g_mmapLength; i++)
				g_sizeOfMemory += mmap[i].len;
			g_bitfieldCount = g_sizeOfMemory >> 20;
			bool found = false;
			// Find a place to put g_physicalMemoryBitfield at.
			for (SIZE_T i = 0; i < g_mmapLength && !found; i++)
			{
				if (mmap[i].len >= (g_bitfieldCount * 8) && mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE)
				{
					g_physicalMemoryBitfield = reinterpret_cast<UINT32_T*>(mmap[i].addr);
					found = true;
				}
			}
			if (!found)
				RestartComputer(); // Oh no!
			for(DWORD* addr = (DWORD*)g_physicalMemoryBitfield; addr < (DWORD*)(g_physicalMemoryBitfield + g_bitfieldCount); addr += 4096)
				utils::dwMemset((DWORD*)kmap_pageTable(addr), 0, 4096 / 4);
			return true;
		}

		PVOID kalloc_physicalPage()
		{
			constexpr SIZE_T maxTries = 8;
			for (SIZE_T i = 0; i < maxTries; i++)
			{
				UINTPTR_T address = 0;
				SIZE_T bit = 0;
				UINT32_T* physicalMemoryBitfield = (UINT32_T*)kmap_pageTable(g_physicalMemoryBitfield + addressToIndex(0, nullptr));
				for (; ; address += 4096)
				{
					if (address == 0x100000)
						address = 0x601000; // Optimization.
					if (isAddressUsed(address))
						continue;
					physicalMemoryBitfield = (UINT32_T*)kmap_pageTable(g_physicalMemoryBitfield + addressToIndex(address, &bit));
					physicalMemoryBitfield += addressToIndex(address, &bit);
					if (!utils::testBitInBitfield(*physicalMemoryBitfield, bit))
						break;
				}
				/*if (isAddressUsed(address))
					continue;*/
				utils::setBitInBitfield(*physicalMemoryBitfield, bit);
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
			UINT32_T* physicalMemoryBitfield = (UINT32_T*)(kmap_pageTable(g_physicalMemoryBitfield + index));
			if (!utils::testBitInBitfield(*physicalMemoryBitfield, bit))
				return 1;
			if (isAddressUsed((UINTPTR_T)_base))
				return 1;
			utils::clearBitInBitfield(*physicalMemoryBitfield, bit);
			return 0;
		}
	}
}