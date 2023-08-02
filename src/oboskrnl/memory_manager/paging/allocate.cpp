/*
	allocate.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/paging/allocate.h>
#include <memory_manager/paging/init.h>
#include <memory_manager/physical.h>

#include <utils/memory.h>

#define indexToAddress(index, pageTableIndex) ((UINTPTR_T)(((index) << 22) + ((pageTableIndex) << 12)))
#define inRange(val, rStart, rEnd) (((UINTPTR_T)(val)) >= ((UINTPTR_T)(rStart)) && ((UINTPTR_T)(val)) <= ((UINTPTR_T)(rEnd)))

namespace obos
{
	namespace memory
	{
		static UINTPTR_T s_lastPageTable[1024] attribute(aligned(4096));
		static UINTPTR_T* kmap_pageTable(PVOID physicalAddress)
		{
			*g_pageDirectory->getPageTableAddress(1023) = (UINTPTR_T)&s_lastPageTable | 3;
			s_lastPageTable[1023] = (UINTPTR_T)physicalAddress;
			s_lastPageTable[1023] |= 3;
			return (UINTPTR_T*)0xFFFFF000;
		}

		PVOID VirtualAlloc(PVOID _base, SIZE_T nPages, utils::RawBitfield flags)
		{
			UINTPTR_T base = GET_FUNC_ADDR(_base);
			// Should we choose a base address?
			if (inRange(base, nullptr, 0x3FFFFF))
			{
				for (int i = 0; i < 1024; i++)
				{
					if (*g_pageDirectory->getPageTableAddress(i) == 2)
					{
						base = indexToAddress(i, 0);
						break;
					}
					for(int j = 0; j < 1024; j++)
					{
						if (kmap_pageTable(g_pageDirectory->getPageTable(i))[j] == 2)
						{
							base = indexToAddress(i, j);
							break;
						}
					}
				}
			}
			base = ROUND_ADDRESS_DOWN(base);
			bool isFree = *g_pageDirectory->getPageTableAddress(PageDirectory::addressToIndex(base)) != 2;
			if (!isFree)
				*g_pageDirectory->getPageTableAddress(PageDirectory::addressToIndex(base)) = (UINTPTR_T)kalloc_physicalPages(1) | 3;
			isFree = !isFree;
			switch (true)
			{
			case true:
			{
				if (isFree)
					break;
				for (UINTPTR_T address = base; address < (base + nPages * 4096); address += 4096)
				{
					UINTPTR_T* pageTable = kmap_pageTable(g_pageDirectory->getPageTable(PageDirectory::addressToIndex(address)));
					if(pageTable[PageDirectory::addressToPageTableIndex(address)] != 2)
					{
						base = 0;
						break;
					}
				}
				break;
			}
			}
			if (!base)
				return nullptr;
			for (UINTPTR_T address = base; address < (base + nPages * 4096); address += 4096)
			{
				UINTPTR_T* pageTable = kmap_pageTable(g_pageDirectory->getPageTable(PageDirectory::addressToIndex(address)));
				pageTable[PageDirectory::addressToPageTableIndex(address)] = (UINTPTR_T)kalloc_physicalPages(1);
				pageTable[PageDirectory::addressToPageTableIndex(address)] |= 1;
				pageTable[PageDirectory::addressToPageTableIndex(address)] |= flags;
				pageTable[PageDirectory::addressToPageTableIndex(address)] &= 0xFFFFFE07;
			}
			return (PVOID)base;
		}
		DWORD VirtualFree(PVOID _base, SIZE_T nPages)
		{
			UINTPTR_T base = ROUND_ADDRESS_DOWN(GET_FUNC_ADDR(_base));
			if (!HasVirtualAddress(_base, nPages) || base < 0x400000)
				return 1;
			utils::memset(_base, 0, nPages << 12);
			for (UINTPTR_T address = base; address < (base + nPages * 4096); address += 4096)
			{
				UINTPTR_T* pageTable = kmap_pageTable(g_pageDirectory->getPageTable(PageDirectory::addressToIndex(address)));
				kfree_physicalPages((PVOID)(pageTable[PageDirectory::addressToPageTableIndex(address)] & 0xFFFFF000), 1);
				pageTable[PageDirectory::addressToPageTableIndex(address)] = 2;
			}
			UINTPTR_T* pageTable = kmap_pageTable(g_pageDirectory->getPageTable(PageDirectory::addressToIndex(base)));
			for (int i = 0; i < 1024; i++)
				if(utils::testBitInBitfield(pageTable[i], 0))
					return 0;
			utils::memset(pageTable, 0, 4096);
			kfree_physicalPages(g_pageDirectory->getPageTable(PageDirectory::addressToIndex(base)), 1);
			kfree_physicalPages(g_pageDirectory->getPageTableAddress(PageDirectory::addressToIndex(base)), 1);
			return 0;
		}
		bool HasVirtualAddress(PCVOID _base, SIZE_T nPages)
		{
			UINTPTR_T base = ROUND_ADDRESS_DOWN(GET_FUNC_ADDR(_base));
			// TODO: Check if this is from a syscall or directly from the kernel.
			/*if (base < 0x400000)
				return false;*/
			if (*g_pageDirectory->getPageTableAddress(PageDirectory::addressToIndex(base)) == 2)
				return false;
			for (UINTPTR_T address = base; address < (base + nPages * 4096); address += 4096)
			{
				UINTPTR_T* pageTable = kmap_pageTable(g_pageDirectory->getPageTable(PageDirectory::addressToIndex(address)));
				if (pageTable[PageDirectory::addressToPageTableIndex(address)] == 2)
					return false;
			}
			return true;
		}
	}
}