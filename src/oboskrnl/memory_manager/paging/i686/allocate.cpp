/*
	oboskrnl/memory_manager/i686/allocate.cpp

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
		UINTPTR_T s_lastPageTable[1024] attribute(aligned(4096));
		UINTPTR_T* kmap_pageTable(PVOID physicalAddress)
		{
			s_lastPageTable[1023] = (UINTPTR_T)physicalAddress;
			s_lastPageTable[1023] |= 3;
			tlbFlush(0xFFFFF000);
			return (UINTPTR_T*)0xFFFFF000;
		}
		void kfree_pageTable()
		{
			s_lastPageTable[1023] = 0;
			tlbFlush(0xFFFFF000);
		}
		UINT32_T* kmap_physical(PVOID _base, SIZE_T nPages, utils::RawBitfield flags, PVOID physicalAddress, bool force)
		{
			UINTPTR_T base = ROUND_ADDRESS_DOWN(GET_FUNC_ADDR(_base));
			bool isFree = utils::testBitInBitfield(*g_pageDirectory->getPageTableAddress(PageDirectory::addressToIndex(base)), 0);
			if (!isFree)
				*g_pageDirectory->getPageTableAddress(PageDirectory::addressToIndex(base)) = (UINTPTR_T)kalloc_physicalPages() | 3;
			isFree = !isFree;
			if (!isFree && !force)
				base = HasVirtualAddress((PCVOID)base, nPages) ? 0 : base;
			if (!base)
				return nullptr;
			for (UINTPTR_T address = base; address < (base + nPages * 4096); address += 4096)
			{
				UINTPTR_T* pageTable = PageDirectory::addressToIndex(address) != 1023 ? kmap_pageTable(g_pageDirectory->getPageTable(PageDirectory::addressToIndex(address))) : 
					(UINTPTR_T*)((UINTPTR_T)&s_lastPageTable);
				pageTable[PageDirectory::addressToPageTableIndex(address)] = (UINTPTR_T)physicalAddress;
				pageTable[PageDirectory::addressToPageTableIndex(address)] |= 1;
				pageTable[PageDirectory::addressToPageTableIndex(address)] |= flags;
				pageTable[PageDirectory::addressToPageTableIndex(address)] &= 0xFFFFF017;
				tlbFlush(0xFFFFF000);
			}
			return (UINT32_T*)base;
		}

		PVOID VirtualAlloc(PVOID _base, SIZE_T nPages, utils::RawBitfield flags)
		{
			UINTPTR_T base = GET_FUNC_ADDR(_base);
			// Should we choose a base address?
			if (!base)
			{
				for (int i = utils::testBitInBitfield(flags, 2) ? 1 : 0x301; i < 1024; i++)
				{
					bool breakOut = false;
					if (!utils::testBitInBitfield(*g_pageDirectory->getPageTableAddress(i), 0))
					{
						base = indexToAddress(i, 0);
						break;
					}
						UINTPTR_T* pageTable = kmap_pageTable(g_pageDirectory->getPageTable(i));
					for(int j = 0; j < 1024; j++)
					{
						if (!utils::testBitInBitfield(pageTable[j], 0))
						{
							base = indexToAddress(i, j);
							breakOut = true;
							break;
						}
					}
					if (breakOut)
						break;
				}
			}
			base = ROUND_ADDRESS_DOWN(base);
			bool isFree = utils::testBitInBitfield(*g_pageDirectory->getPageTableAddress(PageDirectory::addressToIndex(base)), 0);
			if (!isFree)
			{
				*g_pageDirectory->getPageTableAddress(PageDirectory::addressToIndex(base)) = (UINTPTR_T)kalloc_physicalPages() | 3;
				utils::memzero(kmap_pageTable(g_pageDirectory->getPageTable(PageDirectory::addressToIndex(base))), 4096);
			}
			// Make sure the page directory is global if the page was allocated as global, so nothing goes wrong.
			*g_pageDirectory->getPageTableAddress(PageDirectory::addressToIndex(base)) |= (flags & VirtualAllocFlags::GLOBAL);
			isFree = !isFree;
			if (!isFree)
				base = HasVirtualAddress((PCVOID)base, nPages) ? 0 : base;
			if (!base)
				return nullptr;
			for (UINTPTR_T address = base; address < (base + nPages * 4096); address += 4096)
			{
				UINTPTR_T* pageTable = g_pageDirectory->getPageTable(PageDirectory::addressToIndex(address));
				pageTable = kmap_pageTable(pageTable);
				UINTPTR_T entry = 0;
				entry = (UINTPTR_T)kalloc_physicalPages();
				entry |= flags | 1;
				entry &= 0xFFFFF017;
				*(pageTable + PageDirectory::addressToPageTableIndex(address)) = entry;
				tlbFlush(0xFFFFF000);
				tlbFlush(address);
			}
			kfree_pageTable();
			return (PVOID)base;
		}
		DWORD VirtualFree(PVOID _base, SIZE_T nPages)
		{
			UINTPTR_T base = ROUND_ADDRESS_DOWN(GET_FUNC_ADDR(_base));
			if (!HasVirtualAddress(_base, nPages))
				return 1;
			MemoryProtect(_base, 1, VirtualAllocFlags::WRITE_ENABLED);
			utils::memset(_base, 0, nPages << 12);
			for (UINTPTR_T address = base; address < (base + nPages * 4096); address += 4096)
			{
				UINTPTR_T* pageTable = kmap_pageTable(g_pageDirectory->getPageTable(PageDirectory::addressToIndex(address)));
				kfree_physicalPages((PVOID)(pageTable[PageDirectory::addressToPageTableIndex(address)] & 0xFFFFF000), 1);
				pageTable[PageDirectory::addressToPageTableIndex(address)] = 0;
				tlbFlush(0xFFFFF000);
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
			if (base == 0xFFFFF000)
				return utils::testBitInBitfield(s_lastPageTable[1023], 0);
			// TODO: Check if this is from a syscall or directly from the kernel.
			if (!utils::testBitInBitfield(*g_pageDirectory->getPageTableAddress(PageDirectory::addressToIndex(base)), 0))
				return false;
			for (UINTPTR_T address = base; address < (base + nPages * 4096); address += 4096)
			{
				UINTPTR_T* pageTable = kmap_pageTable(g_pageDirectory->getPageTable(PageDirectory::addressToIndex(address)));
				UINTPTR_T physAddress = pageTable[PageDirectory::addressToPageTableIndex(address)];
				if (!utils::testBitInBitfield(physAddress, 0))
					return false;
			}
			return true;
		}
		DWORD MemoryProtect(PVOID _base, SIZE_T nPages, utils::RawBitfield flags)
		{
			UINTPTR_T base = ROUND_ADDRESS_DOWN(GET_FUNC_ADDR(_base));
			if (!HasVirtualAddress(_base, nPages))
				return 1;
			for (UINTPTR_T address = base; address < (base + nPages * 4096); address += 4096)
			{
				UINTPTR_T* pageTable = g_pageDirectory->getPageTable(PageDirectory::addressToIndex(address));
				pageTable = kmap_pageTable(pageTable);
				UINTPTR_T entry = pageTable[PageDirectory::addressToPageTableIndex(address)] & 0xFFFFF000;
				entry |= flags | 1;
				entry &= 0xFFFFF017;
				*(pageTable + PageDirectory::addressToPageTableIndex(address)) = entry;
				tlbFlush(0xFFFFF000);
			}
			*g_pageDirectory->getPageTableAddress(PageDirectory::addressToIndex(base)) |= (flags & VirtualAllocFlags::GLOBAL);
			return 0;
		}
	}
}