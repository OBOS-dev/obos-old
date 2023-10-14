/*
	oboskrnl/arch/x86_64/memory_manager/allocate.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <limine.h>

#include <int.h>
#include <klog.h>

#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>

namespace obos
{
	namespace memory
	{
		static volatile limine_memmap_request mmap_request = {
			.id = LIMINE_MEMMAP_REQUEST,
			.revision = 0
		};
		
		size_t g_ramSize = 0;
		size_t g_bitmapSize = 0;
		uint32_t* g_bitmaps = nullptr;

#define addrToIndex(addr) (addr >> 17)
#define addrToBit(addr) ((addr >> 12) & 0x1f)
		bool getPageStatus(uintptr_t address)
		{
			uint32_t index = addrToIndex(address);
			uint32_t bit   = addrToBit(address);
			uint32_t* bitmap = (uint32_t*)mapPageTable((uintptr_t*)(g_bitmaps + index));
			if (bitmap != (g_bitmaps + index))
			{
				uintptr_t offset = reinterpret_cast<uintptr_t>(g_bitmaps + index) & 0xfff;
				bitmap = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(bitmap) + offset);
			}
			uint32_t bitmap_val = *bitmap;
			return bitmap_val & (1 << bit);
		}
		static bool markPageAs(uintptr_t address, bool newStatus)
		{
			uint32_t index = addrToIndex(address);
			uint32_t bit   = addrToBit(address);
			uint32_t* bitmap = (uint32_t*)mapPageTable((uintptr_t*)(g_bitmaps + index));
			if (bitmap != (g_bitmaps + index))
			{
				uintptr_t offset = reinterpret_cast<uintptr_t>(g_bitmaps + index) & 0xfff;
				bitmap = reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(bitmap) + offset);
			}
			bool ret = getPageStatus(address);
			if(newStatus)
				*bitmap |= (1 << bit);
			else
				*bitmap &= ~(1 << bit);
			return ret;
		}
		void InitializePhysicalMemoryManager()
		{
			for (size_t i = 0; i < mmap_request.response->entry_count; i++)
				g_ramSize += mmap_request.response->entries[i]->length;
			g_bitmapSize = g_ramSize >> 15;
			bool foundSpace = false;
			for (size_t i = 0; i < mmap_request.response->entry_count; i++)
			{
				if (mmap_request.response->entries[i]->type == LIMINE_MEMMAP_USABLE && mmap_request.response->entries[i]->length >= (g_bitmapSize * 8))
				{
					g_bitmaps = reinterpret_cast<uint32_t*>(mmap_request.response->entries[i]->base);
					foundSpace = true;
					break;
				}
			}
			if (!foundSpace)
				logger::panic("Couldn't find enough space to allocate the bitmap for the physical memory manager.\n");
			for (size_t i = 0; i < mmap_request.response->entry_count; i++)
			{
				if (mmap_request.response->entries[i]->type != LIMINE_MEMMAP_USABLE)
				{
					uintptr_t start = mmap_request.response->entries[i]->base & (~0xfff);
					uintptr_t end = (mmap_request.response->entries[i]->base + mmap_request.response->entries[i]->length + 0xfff) & (~0xfff);
					for (uintptr_t currentPage = start; currentPage != end; currentPage += 0x1000)
						markPageAs(currentPage, true);
				}
			}
			uintptr_t bitmap_start = reinterpret_cast<uintptr_t>(g_bitmaps) & (~0xfff);
			uintptr_t bitmap_end = (reinterpret_cast<uintptr_t>(g_bitmaps) + g_bitmapSize + 0xfff) & (~0xfff);
			for (uintptr_t currentPage = bitmap_start; currentPage != bitmap_end; currentPage += 0x1000)
				markPageAs(currentPage, true);
			markPageAs(0, true); // The zero page is allocated.
		}

		uintptr_t allocatePhysicalPage()
		{
			// Find an available page.
			uintptr_t ret = 0x1000;
			for (size_t i = 0; i < g_bitmapSize && getPageStatus(ret); ret += 0x1000, i = addrToIndex(ret));
			if (getPageStatus(ret))
				logger::panic("No more avaliable system memory!\n");
			markPageAs(ret, true);
			return ret;
		}
		bool freePhysicalPage(uintptr_t addr)
		{
			if (!getPageStatus(addr))
				return false;
			markPageAs(addr, false);
			return true;
		}
	}
}