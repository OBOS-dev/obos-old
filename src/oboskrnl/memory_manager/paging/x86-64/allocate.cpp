/*
	oboskrnl/memory_manager/x86-64/allocate.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/paging/allocate.h>
#include <memory_manager/paging/init.h>

#include <memory_manager/physical.h>

#include <types.h>

#include <utils/memory.h>

extern "C" UINTPTR_T boot_page_level4_map;

namespace obos
{
	namespace memory
	{
		// Freestanding. Does not need the physical memory manager, or anything.
		UINTPTR_T* kmap_pageTable(PVOID physicalAddress)
		{
			static UINTPTR_T pageDirectory[512] alignas(4096);
			static UINTPTR_T pageTable[512] alignas(4096);
			(reinterpret_cast<UINTPTR_T***>(&boot_page_level4_map))[0][3] 
			= reinterpret_cast<UINTPTR_T*>(reinterpret_cast<UINTPTR_T>(pageDirectory) | 0x8000000000000003);
			pageDirectory[511] = reinterpret_cast<UINTPTR_T>(pageTable) | 0x8000000000000003;
			pageTable[511] = reinterpret_cast<UINTPTR_T>(physicalAddress) & 0xFFF;
			pageTable[511] |= 3;
			return (UINTPTR_T*)0xFFFFF000;
		}

		static UINTPTR_T* allocatePagingStructures(UINTPTR_T at, UINTPTR_T flags)
		{
			if(!(*g_level4PageMap->getLevel3PageMapAddress(PageMap::computeIndexAtAddress(at, 3))))
			{
				auto addr = kalloc_physicalPage();
				*g_level4PageMap->getLevel3PageMapAddress(PageMap::computeIndexAtAddress(at, 3)) 
					= reinterpret_cast<UINTPTR_T>(addr) | flags;
				utils::memzero(kmap_pageTable(addr), 4096);
				// Fall into the next if statement...
			}
			if (!g_level4PageMap->getLevel3PageMap(PageMap::computeIndexAtAddress(at, 2)))
			{
				auto addr = kalloc_physicalPage();
				UINTPTR_T* l3PageMap = kmap_pageTable(
					reinterpret_cast<PVOID>(*g_level4PageMap->getLevel3PageMap(PageMap::computeIndexAtAddress(at, 3))));
				l3PageMap[PageMap::computeIndexAtAddress(at, 2)] = reinterpret_cast<UINTPTR_T>(addr) | flags;
				utils::memzero(kmap_pageTable(addr), 4096);
				// Fall into the next if statement...
			}
			if (!g_level4PageMap->getPageDirectory(PageMap::computeIndexAtAddress(at, 2), 
												   PageMap::computeIndexAtAddress(at, 1)))
		    {
		 		auto addr = kalloc_physicalPage();
				UINTPTR_T* pageDirectory = g_level4PageMap->getPageDirectoryRealtive(PageMap::computeIndexAtAddress(at, 2), 
												   PageMap::computeIndexAtAddress(at, 1));
				*pageDirectory = reinterpret_cast<UINTPTR_T>(addr) | flags;
				utils::memzero(kmap_pageTable(addr), 4096);
				// Fall into the next if statement...
		    }
			if (!g_level4PageMap->getPageTable(PageMap::computeIndexAtAddress(at, 2), 
											   PageMap::computeIndexAtAddress(at, 1),
											   PageMap::computeIndexAtAddress(at, 0)))
		    {
		 		auto addr = kalloc_physicalPage();
				UINTPTR_T* pageTable = g_level4PageMap->getPageTableRealtive(PageMap::computeIndexAtAddress(at, 2), 
												   PageMap::computeIndexAtAddress(at, 1),
												   PageMap::computeIndexAtAddress(at, 0));
				*pageTable = reinterpret_cast<UINTPTR_T>(addr) | flags;
				utils::memzero(kmap_pageTable(addr), 4096);
		    }
			return kmap_pageTable(g_level4PageMap->getPageTable(PageMap::computeIndexAtAddress(at, 2), 
											   					PageMap::computeIndexAtAddress(at, 1),
											   					PageMap::computeIndexAtAddress(at, 0)));
		}

		PVOID VirtualAlloc(PVOID _base, SIZE_T nPages, UINTPTR_T flags)
		{
			UINTPTR_T base = reinterpret_cast<UINTPTR_T>(_base);
			if (base + (base + nPages * 4096) < base)
				return nullptr; // If the amount of pages overflows the address, return nullptr.
			bool executeDisable = !utils::testBitInBitfield(flags, 63);
			if (utils::testBitInBitfield(flags, 63))
				utils::clearBitInBitfield(flags, 63);
			else
				utils::setBitInBitfield(flags, 63);
			flags &= 0x8000000000000017;
			if (!base)
			{
				// TODO: Determine the address for the caller.
				return nullptr; // Unimplemented.
			}
			for(UINTPTR_T addr = base; addr < (addr + nPages * 4096); addr += 4096)
			{
				UINTPTR_T* pageTable = allocatePagingStructures(addr, flags);
				pageTable += PageMap::computeIndexAtAddress(addr, 0);
				extern UINTPTR_T* g_zeroPage;
				UINTPTR_T entry = reinterpret_cast<UINTPTR_T>(g_zeroPage);
				entry |= 1 | ((UINTPTR_T)1 << 63);
				entry |= (flags & 0b10100);
				if ((flags & 0b10))
				{
					if (executeDisable)
						entry |= ((UINTPTR_T)1 << 58);
					entry |= (flags << 52);
					entry |= (1 << 9); // Set allocate on write.
				}
				*pageTable = entry;
			}
			return (PVOID)base;
		}
	
		DWORD VirtualFree(PVOID _base, SIZE_T nPages)
		{
			UINTPTR_T base =  reinterpret_cast<UINTPTR_T>(_base);
			if (base + (base + nPages * 4096) < base)
				return 1; // If the amount of pages overflows the address, return 1.
			if (!HasVirtualAddress(_base, nPages) || !base)
				return 1;
			for (UINTPTR_T addr = base; base < (base + nPages * 4096); base += 4096)
			{
				UINTPTR_T* pageTable = memory::kmap_pageTable(
					memory::g_level4PageMap->getPageTable(
							memory::PageMap::computeIndexAtAddress(addr & 0xfff, 2), 
							memory::PageMap::computeIndexAtAddress(addr & 0xfff, 1),
							memory::PageMap::computeIndexAtAddress(addr & 0xfff, 0)));
				
			}
			return 0;
		}
		bool HasVirtualAddress(PCVOID _base, SIZE_T nPages)
		{
			UINTPTR_T base =  reinterpret_cast<UINTPTR_T>(_base);
			if (base + (base + nPages * 4096) < base)
				return false;
			for (UINTPTR_T addr = base; base < (base + nPages * 4096); base += 4096)
			{
				if(!g_level4PageMap->getLevel3PageMapAddress(PageMap::computeIndexAtAddress(addr, 3)))
					return false;
				if(!g_level4PageMap->getLevel3PageMap(PageMap::computeIndexAtAddress(addr, 2)))
					return false;
				if(!g_level4PageMap->getPageDirectory(PageMap::computeIndexAtAddress(addr, 2), 
													  PageMap::computeIndexAtAddress(addr, 1)))
					return false;
				if(!g_level4PageMap->getPageTable(PageMap::computeIndexAtAddress(addr, 2), 
												  PageMap::computeIndexAtAddress(addr, 1), 
												  PageMap::computeIndexAtAddress(addr, 0)))
					return false;
				if(!g_level4PageMap->getPageTableEntry(PageMap::computeIndexAtAddress(addr, 2), 
													   PageMap::computeIndexAtAddress(addr, 1), 
													   PageMap::computeIndexAtAddress(addr, 0), PageMap::computeIndexAtAddress(addr, 0)))
					return false;
			}
			return true;
		}
		DWORD MemoryProtect(PVOID _base, SIZE_T nPages, UINTPTR_T flags)
		{
			UINTPTR_T base =  reinterpret_cast<UINTPTR_T>(_base);
			if (base + (base + nPages * 4096) < base)
				return 1; // If the amount of pages overflows the address, return 1.
			if (!HasVirtualAddress(_base, nPages) || !base)
				return 1;
			for (UINTPTR_T addr = base; base < (base + nPages * 4096); base += 4096)
			{
				
			}
		}
	}
}