/*
	oboskrnl/arch/x86_64/memory_manager/allocate.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <limine.h>

#include <int.h>
#include <klog.h>
#include <atomic.h>
#include <memory_manipulation.h>

#include <x86_64-utils/asm.h>

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
		uintptr_t g_memEnd = 0;
		uintptr_t g_lastPageAllocated = 0x1000;
		uintptr_t g_lastPageFreed = 0x1000;
		bool g_physicalMemoryManagerLock = false;

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
			newStatus = !(!newStatus);
			*bitmap = (*bitmap & ~(1 << bit)) | (newStatus << bit);
			/*if(newStatus)
				*bitmap |= (1 << bit);
			else
				*bitmap &= ~(1 << bit);*/
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
				if (mmap_request.response->entries[i]->type == LIMINE_MEMMAP_USABLE && mmap_request.response->entries[i]->length >= (g_bitmapSize * 4))
				{
					g_bitmaps = reinterpret_cast<uint32_t*>(mmap_request.response->entries[i]->base);
					foundSpace = true;
					break;
				}
			}
			g_memEnd = mmap_request.response->entries[mmap_request.response->entry_count - 1]->base + mmap_request.response->entries[mmap_request.response->entry_count - 1]->length;
			if (!foundSpace)
				logger::panic("Couldn't find enough space to allocate the bitmap for the physical memory manager.\n");
			utils::memzero(g_bitmaps, g_bitmapSize);
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
			uintptr_t bitmap_end = (reinterpret_cast<uintptr_t>(g_bitmaps + g_bitmapSize) + 0xfff) & (~0xfff);
			for (uintptr_t currentPage = bitmap_start; currentPage != bitmap_end; currentPage += 0x1000)
				markPageAs(currentPage, true);
			markPageAs(0, true); // The zero page is allocated.
			g_lastPageAllocated = g_lastPageFreed = bitmap_end;
		}

		uintptr_t allocatePhysicalPage()
		{
			while (atomic_test(&g_physicalMemoryManagerLock));
			atomic_set(&g_physicalMemoryManagerLock);
			// Find an available page.
			uintptr_t ret = 0;
			bool bAddr = (g_lastPageFreed < g_lastPageAllocated && !getPageStatus(g_lastPageFreed));
			ret = g_lastPageFreed * bAddr;
			//set_if_zero(&ret, g_lastPageFreed);
			uintptr_t newRet = ((!bAddr) * g_lastPageAllocated);
			bAddr = (bool)newRet;
			newRet += ((uint32_t)getPageStatus(g_lastPageAllocated) * 0x1000 * bAddr);
			//if (!ret)
			//	ret = newRet;
			//asm volatile("test %0, %0; cmove %1, %0;" : "=r"(ret) : "a"(newRet));
			// Use cmove as an optimization.
			set_if_zero(&ret, newRet);
			if (!newRet && !ret)
				for (ret = 0x1000; ret < g_memEnd && getPageStatus(ret); ret += 0x1000);
			if (getPageStatus(ret))
				for (ret = g_lastPageFreed; ret < g_memEnd && getPageStatus(ret); ret += 0x1000);
			if (ret == g_memEnd)
			{
				for (ret = 0x1000; ret < g_memEnd && getPageStatus(ret); ret += 0x1000);
				if (ret == g_memEnd)
					logger::panic("No more avaliable system memory!\n");
			}
			markPageAs(ret, true);
			g_lastPageAllocated = ret;
			atomic_clear(&g_physicalMemoryManagerLock);
			return ret;
		}
		bool freePhysicalPage(uintptr_t addr)
		{
			while (atomic_test(&g_physicalMemoryManagerLock));
			atomic_set(&g_physicalMemoryManagerLock);
			addr &= ~(0xfff);
			g_lastPageFreed = markPageAs(addr, !((bool)addr)) * addr;
			atomic_clear(&g_physicalMemoryManagerLock);
			return (bool)addr;
		}
	}
}