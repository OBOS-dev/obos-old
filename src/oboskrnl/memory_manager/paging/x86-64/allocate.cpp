/*
	oboskrnl/memory_manager/x86-64/allocate.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/paging/allocate.h>
#include <memory_manager/paging/init.h>

#include <memory_manager/physical.h>

#include <types.h>

#include <utils/memory.h>

#include <process/process.h>

#include <multitasking/multitasking.h>

extern "C" UINTPTR_T boot_page_level3_map;

namespace obos
{
	extern char kernelStart;
	namespace memory
	{
		// Freestanding. Does not need the physical memory manager, or anything.
		UINTPTR_T* kmap_pageTable(PVOID physicalAddress)
		{
			static UINTPTR_T pageDirectory[512] alignas(4096);
			static UINTPTR_T pageTable[512] alignas(4096);
			UINTPTR_T flags = static_cast<UINTPTR_T>(CPUSupportsExecuteDisable()) << 63;
			flags |= 3;
			reinterpret_cast<UINTPTR_T*>((UINTPTR_T)(g_level4PageMap->getPageMap()[0]) & 0xFFFFFFFFFF000)[3] = 0;
			pageDirectory[511] = 0;
			pageTable[511] = 0;

			pageDirectory[511] = (reinterpret_cast<UINTPTR_T>(pageTable) - reinterpret_cast<UINTPTR_T>(&kernelStart)) | flags;
			pageTable[511] = reinterpret_cast<UINTPTR_T>(physicalAddress);
			pageTable[511] &= (~0xFFF);
			pageTable[511] |= flags;
			reinterpret_cast<UINTPTR_T*>((UINTPTR_T)(g_level4PageMap->getPageMap()[0]) & 0xFFFFFFFFFF000)[3]
				= (reinterpret_cast<UINTPTR_T>(pageDirectory) - reinterpret_cast<UINTPTR_T>(&kernelStart)) | flags;
			tlbFlush(0xFFFFF000);
			return (UINTPTR_T*)0xFFFFF000;
		}

		UINTPTR_T* allocatePagingStructures(UINTPTR_T at, UINTPTR_T flags)
		{
			flags |= 1;
			if (!(*g_level4PageMap->getLevel3PageMapAddress(PageMap::computeIndexAtAddress(at, 3))))
			{
				auto addr = kalloc_physicalPage();
				*g_level4PageMap->getLevel3PageMapAddress(PageMap::computeIndexAtAddress(at, 3))
					= reinterpret_cast<UINTPTR_T>(addr) | flags;
				utils::memzero(kmap_pageTable(addr), 4096);
				// Fall into the next if statement...
			}
			if (!g_level4PageMap->getLevel3PageMap(PageMap::computeIndexAtAddress(at, 3)))
			{
				auto addr = kalloc_physicalPage();
				UINTPTR_T* l3PageMap = kmap_pageTable(
					reinterpret_cast<PVOID>(*g_level4PageMap->getLevel3PageMap(PageMap::computeIndexAtAddress(at, 3))));
				l3PageMap[PageMap::computeIndexAtAddress(at, 2)] = reinterpret_cast<UINTPTR_T>(addr) | flags;
				utils::memzero(kmap_pageTable(addr), 4096);
				// Fall into the next if statement...
			}
			if (!g_level4PageMap->getPageDirectory(PageMap::computeIndexAtAddress(at, 3),
				PageMap::computeIndexAtAddress(at, 2)))
			{
				auto addr = kalloc_physicalPage();
				UINTPTR_T* pageDirectory = g_level4PageMap->getPageDirectoryRealtive(PageMap::computeIndexAtAddress(at, 3),
					PageMap::computeIndexAtAddress(at, 2));
				*pageDirectory = reinterpret_cast<UINTPTR_T>(addr) | flags;
				utils::memzero(kmap_pageTable(addr), 4096);
				// Fall into the next if statement...
			}
			if (!(*g_level4PageMap->getPageTableRealtive(PageMap::computeIndexAtAddress(at, 3),
				PageMap::computeIndexAtAddress(at, 2),
				PageMap::computeIndexAtAddress(at, 1))))
			{
				auto addr = kalloc_physicalPage();
				UINTPTR_T* pageTable = g_level4PageMap->getPageTableRealtive(PageMap::computeIndexAtAddress(at, 3),
					PageMap::computeIndexAtAddress(at, 2),
					PageMap::computeIndexAtAddress(at, 1));
				*pageTable = reinterpret_cast<UINTPTR_T>(addr) | flags;
				utils::memzero(kmap_pageTable(addr), 4096);
			}
			return kmap_pageTable(g_level4PageMap->getPageTable(PageMap::computeIndexAtAddress(at, 3),
				PageMap::computeIndexAtAddress(at, 2),
				PageMap::computeIndexAtAddress(at, 1)));
		}

		PVOID VirtualAlloc(PVOID _base, SIZE_T nPages, UINTPTR_T flags, bool commit)
		{
			UINTPTR_T base = reinterpret_cast<UINTPTR_T>(_base);
			if ((base + nPages * 4096) < base)
				return nullptr; // If the amount of pages overflows the address, return nullptr.
			bool executeDisable = !utils::testBitInBitfield(flags, 63) && CPUSupportsExecuteDisable();
			if (utils::testBitInBitfield(flags, 63) || !CPUSupportsExecuteDisable())
				utils::clearBitInBitfield(flags, 63);
			else
				utils::setBitInBitfield(flags, 63);
			// To prevent future problems, do this.
			UINTPTR_T pageStructureFlags =
				VirtualAllocFlags::GLOBAL |
				(UINTPTR_T)CPUSupportsExecuteDisable() << 63 |
				VirtualAllocFlags::WRITE_ENABLED |
				1;
			flags &= 0x8000000000000017;
			if (!base)
			{
				for (base = 0x600000; HasVirtualAddress(reinterpret_cast<PCVOID>(base), nPages); base += 4096);
				if (!base)
					return nullptr;
			}
			if (HasVirtualAddress(reinterpret_cast<PCVOID>(base), nPages))
				return nullptr;
			for (UINTPTR_T addr = base; addr < (base + nPages * 4096); addr += 4096)
			{
				UINTPTR_T entry = 0;
				if (!commit)
				{
					extern UINTPTR_T* g_zeroPage;
					entry = reinterpret_cast<UINTPTR_T>(g_zeroPage);
					entry |= 1 | ((UINTPTR_T)CPUSupportsExecuteDisable() << 63);
					/*if (!CPUSupportsExecuteDisable())
						utils::clearBitInBitfield(entry, 63);*/
					entry |= (flags & 0b10100);
					if ((flags & 0b10))
					{
						if (executeDisable)
							entry |= ((UINTPTR_T)1 << 58);
						entry |= (flags << 52);
						entry |= (1 << 9); // Set allocate on write.
					}
				}
				else
				{
					entry = reinterpret_cast<UINTPTR_T>(kalloc_physicalPage());
					entry |= 1 | flags;
				}
				UINTPTR_T* pageTable = allocatePagingStructures(addr, pageStructureFlags);
				tlbFlush((UINTPTR_T)pageTable);
				pageTable += PageMap::computeIndexAtAddress(addr, 0);
				*pageTable = entry;
			}
			if (multitasking::g_initialized && multitasking::g_currentThread->owner && multitasking::g_currentThread->owner->pid != 0 && base < 0xFFFFFFFF80000000)
			{
				process::Process::allocatedBlock* block = new process::Process::allocatedBlock{};
				block->start = (PBYTE)base;
				block->size = nPages;
				process::Process* currentProcess = multitasking::g_currentThread->owner;
				list_rpush(currentProcess->allocatedBlocks, list_node_new(block));
			}
			
			return (PVOID)base;
		}

		DWORD VirtualFree(PVOID _base, SIZE_T nPages)
		{
			UINTPTR_T base = reinterpret_cast<UINTPTR_T>(_base);
			if ((base + nPages * 4096) < base)
				return 1; // If the amount of pages overflows the address, return 1.
			if (!HasVirtualAddress(_base, nPages, true) || !base)
				return 1;
			if (multitasking::g_initialized && multitasking::g_currentThread->owner && multitasking::g_currentThread->owner->pid != 0 && base < 0xFFFFFFFF80000000)
			{
				process::Process::allocatedBlock block;
				block.start = (PBYTE)base;
				block.size = nPages;
				process::Process* currentProcess = multitasking::g_currentThread->owner;
				list_node_t* currentNode = list_find(currentProcess->allocatedBlocks, &block);
				delete (process::Process::allocatedBlock*)currentNode->val;
				list_remove(currentProcess->allocatedBlocks, currentNode);
			}
			for (UINTPTR_T addr = base; addr < (base + nPages * 4096); addr += 4096)
			{
				UINTPTR_T* pageTable = memory::kmap_pageTable(
					memory::g_level4PageMap->getPageTable(
						memory::PageMap::computeIndexAtAddress(addr & (~0xfff), 3),
						memory::PageMap::computeIndexAtAddress(addr & (~0xfff), 2),
						memory::PageMap::computeIndexAtAddress(addr & (~0xfff), 1)));
				auto pageTableIndex = memory::PageMap::computeIndexAtAddress(addr & (~0xfff), 0);
				if (!utils::testBitInBitfield(pageTable[pageTableIndex], 9))
				{
					// Free the physical page.
					UINTPTR_T phys = pageTable[pageTableIndex] & 0x1FFFFEFFFFF000;
					kfree_physicalPage(reinterpret_cast<PVOID>(phys));
				}
				pageTable[pageTableIndex] = 0;
			}
			return 0;
		}
		bool HasVirtualAddress(PCVOID _base, SIZE_T nPages, bool checkAllocationSize)
		{
			UINTPTR_T base = reinterpret_cast<UINTPTR_T>(_base) & (~0xFFF);
			if ((base + nPages * 4096) < base)
				return false;
			for (UINTPTR_T addr = base; addr < (base + nPages * 4096); addr += 4096)
			{
				if (!g_level4PageMap->getLevel3PageMapAddress(PageMap::computeIndexAtAddress(addr, 3)))
					return false;
				if (!g_level4PageMap->getLevel3PageMap(PageMap::computeIndexAtAddress(addr, 3)))
					return false;
				if (!g_level4PageMap->getPageDirectory(PageMap::computeIndexAtAddress(addr, 3),
					PageMap::computeIndexAtAddress(addr, 2)))
					return false;
				if (!g_level4PageMap->getPageTable(PageMap::computeIndexAtAddress(addr, 3),
					PageMap::computeIndexAtAddress(addr, 2),
					PageMap::computeIndexAtAddress(addr, 1)))
					return false;
				if (!g_level4PageMap->getPageTableEntry(PageMap::computeIndexAtAddress(addr, 3),
					PageMap::computeIndexAtAddress(addr, 2),
					PageMap::computeIndexAtAddress(addr, 1), PageMap::computeIndexAtAddress(addr, 0)))
					return false;
			}
			if (multitasking::g_initialized && multitasking::g_currentThread->owner && multitasking::g_currentThread->owner->pid != 0 && base < 0xFFFFFFFF80000000 && checkAllocationSize)
			{
				process::Process::allocatedBlock block;
				block.start = (PBYTE)base;
				block.size = nPages;
				process::Process* currentProcess = multitasking::g_currentThread->owner;
				list_node_t* currentNode = list_find(currentProcess->allocatedBlocks, &block);
				return currentNode != nullptr;
			}
			return true;
		}
		DWORD MemoryProtect(PVOID _base, SIZE_T nPages, UINTPTR_T flags, bool checkAllocationSize)
		{
			UINTPTR_T base = reinterpret_cast<UINTPTR_T>(_base) & (~0xFFF);
			if ((base + nPages * 4096) < base)
				return 1; // If the amount of pages overflows the address, return 1.
			if (!HasVirtualAddress(_base, nPages, checkAllocationSize) || !base)
				return 1;
			if (utils::testBitInBitfield(flags, 63) || !CPUSupportsExecuteDisable())
				utils::clearBitInBitfield(flags, 63);
			else
				utils::setBitInBitfield(flags, 63);
			flags &= 0x8000000000000017;
			for (UINTPTR_T addr = base; addr < (base + nPages * 4096); addr += 4096)
			{
				UINTPTR_T entry = memory::g_level4PageMap->getRealPageTableEntry(
					memory::PageMap::computeIndexAtAddress(addr & (~0xfff), 3),
					memory::PageMap::computeIndexAtAddress(addr & (~0xfff), 2),
					memory::PageMap::computeIndexAtAddress(addr & (~0xfff), 1),
					memory::PageMap::computeIndexAtAddress(addr & (~0xfff), 0));
				if (!utils::testBitInBitfield(entry, 9) && utils::testBitInBitfield(entry, 1))
				{
					// The page is commited and write enabled, so change the flags directly.
					entry &= 0xFFFFFFFFFF000;
					entry |= flags | 1;
				}
				else if (utils::testBitInBitfield(entry, 9))
				{
					// Page isn't commited, so change the flags at bit offset 52.
					entry &= 0xF80FFFFFFFFFFFFF; // Clear bits 52-58
					entry |= (flags << 52);
					if (!utils::testBitInBitfield(flags, 63) && CPUSupportsExecuteDisable())
						utils::setBitInBitfield(entry, 63); // If EXECUTE_ENABLE is cleared in flags, set XD (execute disable) in entry.
				}
				kmap_pageTable(memory::g_level4PageMap->getPageTable(
					memory::PageMap::computeIndexAtAddress(addr & (~0xfff), 3),
					memory::PageMap::computeIndexAtAddress(addr & (~0xfff), 2),
					memory::PageMap::computeIndexAtAddress(addr & (~0xfff), 1)))[memory::PageMap::computeIndexAtAddress(addr & (~0xfff), 0)] = entry;
			}
			return 0;
		}

		UINT32_T* kmap_physical(PVOID _base, UINTPTR_T flags, PVOID physicalAddress, bool force)
		{
			UINTPTR_T base = reinterpret_cast<UINTPTR_T>(_base);
			if (base == 0xFFFFFFFFFFFFFFFF)
				return nullptr; // If the amount of pages overflows the address, return nullptr.
			if (utils::testBitInBitfield(flags, 63) || !CPUSupportsExecuteDisable())
				utils::clearBitInBitfield(flags, 63);
			else
				utils::setBitInBitfield(flags, 63);
			UINTPTR_T pageStructureFlags =
				VirtualAllocFlags::GLOBAL |
				VirtualAllocFlags::WRITE_ENABLED |
				1;
			if (CPUSupportsExecuteDisable())
				pageStructureFlags |= VirtualAllocFlags::EXECUTE_ENABLE;
			flags &= 0x87F0000000000217;
			if (!base)
				return nullptr; // Not supported.
			if(!force)
				if (HasVirtualAddress(_base, 1))
					return nullptr;
			UINTPTR_T* pageTable = allocatePagingStructures(base, pageStructureFlags);
			pageTable += PageMap::computeIndexAtAddress(base, 0);
			UINTPTR_T entry = reinterpret_cast<UINTPTR_T>(physicalAddress);
			entry |= flags | 1;
			*pageTable = entry;
			return (UINT32_T*)base;
		}
	}
}