/*
	physical.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/physical.h>

#include <klog.h>

#include <boot/multiboot.h>

// Trigger warning.
extern "C" char _ZN4obos5utils8Bitfield6setBitEh;
// Trigger warning.
static auto obos_utils_bitfield_setBit = (void(*)(obos::utils::Bitfield*, UINT8_T))GET_FUNC_ADDR(&_ZN4obos5utils8Bitfield6setBitEh);

namespace obos
{
	extern multiboot_info_t* g_multibootInfo;
	namespace memory
	{
		utils::Bitfield g_availablePages[g_countPages / 32];
		SIZE_T g_physicalMemorySize = 0;
		bool g_physicalMemoryManagerInitialized = false;

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

		bool InitializePhysicalMemoryManager()
		{
			if ((g_multibootInfo->flags & MULTIBOOT_INFO_MEM_MAP) != MULTIBOOT_INFO_MEM_MAP)
				obos::kpanic("No memory map from the bootloader.");
			if (g_physicalMemoryManagerInitialized)
				return false;
			for (SIZE_T i = 0; i < g_countPages / 32; i++)
				g_availablePages[i] = utils::Bitfield();
			UINT32_T bit = 0;
			for (multiboot_memory_map_t* current = (multiboot_memory_map_t*)g_multibootInfo->mmap_addr;
				current != (multiboot_memory_map_t*)(g_multibootInfo->mmap_addr + g_multibootInfo->mmap_length);
				current++)
			{
				if (current->type != MULTIBOOT_MEMORY_AVAILABLE || current->addr == 0x00000000)
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
						obos_utils_bitfield_setBit(obj, bit);
					}
				}
				g_physicalMemorySize += current->len;
			}
			// Takes too long.
			/*const UINTPTR_T begin = ((g_physicalMemorySize >> 12) + 1) << 12;
			for (UINTPTR_T addr = begin; addr < 0xFFFFFFFF && addr >= begin; addr += 4096, bit = 0)
			{
				auto index = addressToIndex(addr, &bit);
				auto* obj = &g_availablePages[index];
				if (*(UINT32_T*)obj == 0xFFFFFFFF)
					continue;
				obos_utils_bitfield_setBit(obj, bit);
			}*/
			return g_physicalMemoryManagerInitialized = true;
		}

		static SIZE_T s_avaliablePagesIndex = 0;
		static SIZE_T s_avaliablePagesBit = 0;

		PVOID kalloc_physicalPages(SIZE_T nPages)
		{
			for (int tries = 0; tries < 8 && g_physicalMemoryManagerInitialized; tries++)
			{
				for (; s_avaliablePagesIndex < g_countPages / 32; s_avaliablePagesIndex++)
				{
					if (g_availablePages[s_avaliablePagesIndex])
						continue;
					for (; s_avaliablePagesBit < 32; s_avaliablePagesBit++)
					{
						if (!g_availablePages[s_avaliablePagesIndex][s_avaliablePagesBit])
							break;
					}
					if(!g_availablePages[s_avaliablePagesIndex][s_avaliablePagesBit])
						break;
				}
				UINTPTR_T address = indexToAddress(s_avaliablePagesIndex, s_avaliablePagesBit);
				UINT32_T bit = 0;
				for (SIZE_T i = 0; i < nPages; i++, address += 4096)
				{
					if (g_availablePages[addressToIndex(address, &bit)][bit])
					{
						address = 0;
						break;
					}
				}
				if (!address ||
					 address > ((g_physicalMemorySize >> 12) << 12))
					continue;
				UINTPTR_T base = 0;
				for (base = address = indexToAddress(s_avaliablePagesIndex, s_avaliablePagesBit);
					address != (base + nPages * 4096);
					address += 4096)
					g_availablePages[s_avaliablePagesIndex = addressToIndex(address, &s_avaliablePagesBit)].setBit(s_avaliablePagesBit);
				return (PVOID)base;
			}
			return nullptr;
		}
	}
	// namespace memory
}
// namespace obos
