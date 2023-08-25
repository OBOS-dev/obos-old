/*
	oboskrnl/memory_manager/i686/physical.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/physical.h>

#include <klog.h>
#include <inline-asm.h>

#include <boot/multiboot.h>
#include <boot/boot.h>

//// Trigger warning.
//extern "C" char _ZN4obos5utils8Bitfield6setBitEh;
//// Trigger warning.
//static auto obos_utils_bitfield_setBit = (void(*)(obos::utils::Bitfield*, UINT8_T))GET_FUNC_ADDR(&_ZN4obos5utils8Bitfield6setBitEh);

#define inRange(val, rStart, rEnd) (((UINTPTR_T)(val)) >= ((UINTPTR_T)(rStart)) && val <= ((UINTPTR_T)(rEnd)))

namespace obos
{
	namespace memory
	{
		utils::RawBitfield g_availablePages[g_countPages / 32];
		SIZE_T g_physicalMemorySize = 0;
		bool g_physicalMemoryManagerInitialized = false;

		static SIZE_T s_availablePagesIndex = 64;
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
			if (address < 0x400000)
				return true;
			for (multiboot_memory_map_t* current = (multiboot_memory_map_t*)g_multibootInfo->mmap_addr;
				current != (multiboot_memory_map_t*)(g_multibootInfo->mmap_addr + g_multibootInfo->mmap_length);
				current++)
			{
				if (current->type != MULTIBOOT_MEMORY_AVAILABLE && current->addr >= (UINTPTR_T)&endImage)
					if (inRange(address, current->addr, current->addr + ((((current->len >> 12) + 1) << 12))))
						return true;
			}
			return false;
		}


		bool InitializePhysicalMemoryManager()
		{
			if ((g_multibootInfo->flags & MULTIBOOT_INFO_MEM_MAP) != MULTIBOOT_INFO_MEM_MAP)
				obos::kpanic(nullptr, getEIP(), kpanic_format("No memory map from the bootloader."));
			if (g_physicalMemoryManagerInitialized)
				return false;
			for (SIZE_T i = 0; i < g_countPages / 32; i++)
				g_availablePages[i] = 0;
			UINT32_T bit = 0;
			for (multiboot_memory_map_t* current = (multiboot_memory_map_t*)g_multibootInfo->mmap_addr;
				current != (multiboot_memory_map_t*)(g_multibootInfo->mmap_addr + g_multibootInfo->mmap_length);
				current++)
			{
				if (current->type != MULTIBOOT_MEMORY_AVAILABLE && current->addr >= (UINTPTR_T)&endImage)
				{
					UINT64_T limit = (((current->len >> 12) + 1) << 12) + current->addr;
					if (limit > 0xFFFFFFFF)
						limit = 0xFFFFFFFF;
					// Mark the pages as in use.
					for (UINTPTR_T addr = current->addr;
						addr < (UINTPTR_T)limit && addr >= current->addr; 
						addr += 4096, bit = 0)
					{
						auto index = addressToIndex(addr, &bit);
						auto* obj = &g_availablePages[index];
						// Trigger warning.
						if (*(UINT32_T*)obj == 0xFFFFFFFF)
							continue;
						utils::setBitInBitfield(*obj, bit);
					}
				}
				g_physicalMemorySize += current->len;
			}
			for (UINTPTR_T addr = 0; addr < (UINTPTR_T)&endImage; addr += 4096, bit = 0)
			{
				auto index = addressToIndex(addr, &bit);
				auto* obj = &g_availablePages[index];
				if (*(UINT32_T*)obj == 0xFFFFFFFF)
					continue;
				utils::setBitInBitfield(*obj, bit);
			}
			s_availablePagesIndex = addressToIndex((UINTPTR_T)&endImage, &s_availablePagesBit);
			return g_physicalMemoryManagerInitialized = true;
		}

		PVOID kalloc_physicalPages()
		{
			for (int tries = 0; tries < 8 && g_physicalMemoryManagerInitialized; tries++)
			{
				if (tries)
				{
					s_availablePagesBit++;
					if (s_availablePagesBit > 31)
					{
						s_availablePagesBit = 0;
						s_availablePagesIndex++;
					}
				}/*
				if (s_availablePages && s_availablePages < nPages)
				{
					s_availablePagesBit += s_availablePages;
					if (s_availablePagesBit > 31)
					{
						s_availablePagesBit -= 32;
						s_availablePagesIndex++;
					}
				}*/
				for (; s_availablePagesIndex < g_countPages / 32; s_availablePagesIndex++)
				{
					if (g_availablePages[s_availablePagesIndex] == 0xFFFFFFFF)
						continue;
					for (; s_availablePagesBit < 32; s_availablePagesBit++)
					{
						if (!utils::testBitInBitfield(g_availablePages[s_availablePagesIndex], s_availablePagesBit))
							break;
					}
					if(!utils::testBitInBitfield(g_availablePages[s_availablePagesIndex], s_availablePagesBit))
						break;
				}
				UINTPTR_T address = indexToAddress(s_availablePagesIndex, s_availablePagesBit);
				UINT32_T bit = 0;
				if (isAddressUsed(address))
					continue;
				if (!address)
					continue;
				address = indexToAddress(s_availablePagesIndex, s_availablePagesBit);
				utils::setBitInBitfield(g_availablePages[s_availablePagesIndex], s_availablePagesBit);
				if (!address)
					continue;
				return (PVOID)address;
			}
			return nullptr;
		}
		INT kfree_physicalPages(PVOID _base)
		{
			UINTPTR_T base = (UINTPTR_T)_base;
			base = (base >> 12) << 12;
			UINTPTR_T end = base + (nPages << 12);
			UINT32_T bit = 0;
			UINT32_T index = addressToIndex(base, &bit);
			if (!nPages || isAddressUsed(base))
				return -1;
			for (UINTPTR_T address = base; address < end; address += 4096, index = addressToIndex(base, &bit))
				if (!utils::testBitInBitfield(g_availablePages[index], bit))
					return -1;
			s_availablePagesIndex = addressToIndex(base, &s_availablePagesBit);
			//s_availablePages = nPages;
			for (UINTPTR_T address = base; address < end; address += 4096, index = addressToIndex(base, &bit))
				utils::clearBitInBitfield(g_availablePages[index], bit);
			return nPages;
		}
	}
	// namespace memory
}
// namespace obos
