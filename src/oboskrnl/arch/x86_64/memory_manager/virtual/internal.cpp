/*
	 oboskrnl/arch/x86_64/memory_manager/virtual/internal.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>

#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>
#include <arch/x86_64/memory_manager/virtual/internal.h>

#include <multitasking/process/process.h>

#include <multitasking/cpu_local.h>

#include <x86_64-utils/asm.h>

namespace obos
{
	namespace memory
	{
		uintptr_t DecodeProtectionFlags(uintptr_t _flags)
		{
			_flags &= PROT_ALL_BITS_SET;
			uintptr_t flags = 0;
			if (!(_flags & PROT_READ_ONLY))
				flags |= ((uintptr_t)1 << 1);
			if (!(_flags & PROT_CAN_EXECUTE) && CPUSupportsExecuteDisable())
				flags |= ((uintptr_t)1 << 63);
			if (_flags & PROT_DISABLE_CACHE)
				flags |= ((uintptr_t)1 << 4);
			if (_flags & PROT_USER_MODE_ACCESS)
				flags |= ((uintptr_t)1 << 2);
			if (_flags & PROT_IS_PRESENT)
				flags |= ((uintptr_t)1 << 0);
			return flags;
		}
		uintptr_t* allocatePagingStructures(uintptr_t address, PageMap* pageMap, uintptr_t flags)
		{
			if (!pageMap->getL4PageMapEntryAt(address))
			{
				uintptr_t entry = allocatePhysicalPage();
				uintptr_t _flags = saveFlagsAndCLI();
				uintptr_t* _pageMap = mapPageTable(pageMap->getPageMap());
				_pageMap[PageMap::addressToIndex(address, 3)] = entry | flags;
				utils::memzero(mapPageTable((uintptr_t*)entry), 4096);
				restorePreviousInterruptStatus(_flags);
			}
			else
			{
				uintptr_t entry = (uintptr_t)pageMap->getL4PageMapEntryAt(address);
				if ((entry & ((uintptr_t)1 << 63)) && !(flags & ((uintptr_t)1 << 63)))
					entry &= ~((uintptr_t)1 << 63);
				if (!(entry & ((uintptr_t)1 << 2)) && (flags & ((uintptr_t)1 << 2)))
					entry |= ((uintptr_t)1 << 2);
				if (!(entry & ((uintptr_t)1 << 1)) && (flags & ((uintptr_t)1 << 1)))
					entry |= ((uintptr_t)1 << 1);
				// See the Intel SDM Volume 3A Section 4.9 for why these lines are commented.
				/*if (!(entry & ((uintptr_t)1 << 4)) && (flags & ((uintptr_t)1 << 1)))
					entry |= ((uintptr_t)1 << 4);*/
				uintptr_t _flags = saveFlagsAndCLI();
				uintptr_t* _pageMap = mapPageTable(pageMap->getPageMap());
				_pageMap[PageMap::addressToIndex(address, 3)] = entry;
				restorePreviousInterruptStatus(_flags);
			}
			if (!pageMap->getL3PageMapEntryAt(address))
			{
				uintptr_t entry = allocatePhysicalPage();
				uintptr_t _flags = saveFlagsAndCLI();
				uintptr_t physAddr = (uintptr_t)pageMap->getL4PageMapEntryAt(address) & g_physAddrMask;
				OBOS_ASSERTP(physAddr < ((uintptr_t)1 << GetPhysicalAddressBits()), "physAddr: 0x%p", , physAddr);
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>(physAddr));
				_pageMap[PageMap::addressToIndex(address, 2)] = entry | flags;
				utils::memzero(mapPageTable((uintptr_t*)entry), 4096);
				restorePreviousInterruptStatus(_flags);
			}
			else
			{
				uintptr_t entry = (uintptr_t)pageMap->getL3PageMapEntryAt(address);
				if ((entry & ((uintptr_t)1 << 63)) && !(flags & ((uintptr_t)1 << 63)))
					entry &= ~((uintptr_t)1 << 63);
				if (!(entry & ((uintptr_t)1 << 2)) && (flags & ((uintptr_t)1 << 2)))
					entry |= ((uintptr_t)1 << 2);
				if (!(entry & ((uintptr_t)1 << 1)) && (flags & ((uintptr_t)1 << 1)))
					entry |= ((uintptr_t)1 << 1);
				uintptr_t _flags = saveFlagsAndCLI();
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)pageMap->getL4PageMapEntryAt(address) & g_physAddrMask));
				_pageMap[PageMap::addressToIndex(address, 2)] = entry;
				restorePreviousInterruptStatus(_flags);
			}
			if (!pageMap->getL2PageMapEntryAt(address))
			{
				uintptr_t entry = allocatePhysicalPage();
				uintptr_t _flags = saveFlagsAndCLI();
				uintptr_t physAddr = (uintptr_t)pageMap->getL3PageMapEntryAt(address) & g_physAddrMask;
				OBOS_ASSERTP(physAddr < ((uintptr_t)1 << GetPhysicalAddressBits()), "physAddr: 0x%p", , physAddr);
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>(physAddr));
				_pageMap[PageMap::addressToIndex(address, 1)] = entry | flags;
				utils::memzero(mapPageTable((uintptr_t*)entry), 4096);
				restorePreviousInterruptStatus(_flags);
			}
			else
			{
				uintptr_t entry = (uintptr_t)pageMap->getL2PageMapEntryAt(address);
				if ((entry & ((uintptr_t)1 << 63)) && !(flags & ((uintptr_t)1 << 63)))
					entry &= ~((uintptr_t)1 << 63);
				if (!(entry & ((uintptr_t)1 << 2)) && (flags & ((uintptr_t)1 << 2)))
					entry |= ((uintptr_t)1 << 2);
				if (!(entry & ((uintptr_t)1 << 1)) && (flags & ((uintptr_t)1 << 1)))
					entry |= ((uintptr_t)1 << 1);
				uintptr_t _flags = saveFlagsAndCLI();
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)pageMap->getL3PageMapEntryAt(address) & g_physAddrMask));
				_pageMap[PageMap::addressToIndex(address, 1)] = entry;
				restorePreviousInterruptStatus(_flags);
			}
			uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)pageMap->getL2PageMapEntryAt(address) & g_physAddrMask));
			return _pageMap;
		}

		bool PagesAllocated(const void* _base, size_t nPages, PageMap* pageMap)
		{
			uintptr_t base = (uintptr_t)_base & ~0xfff;
			for (uintptr_t addr = base; addr != (base + nPages * 4096); addr += 4096)
			{
				if (!((uintptr_t)pageMap->getL4PageMapEntryAt(addr) & 1))
					return false;
				if (!((uintptr_t)pageMap->getL3PageMapEntryAt(addr) & 1))
					return false;
				if (!((uintptr_t)pageMap->getL2PageMapEntryAt(addr) & 1))
					return false;
				uintptr_t l1Entry = (uintptr_t)pageMap->getL1PageMapEntryAt(addr);
				if (!(l1Entry & 1) && !(l1Entry & ((uintptr_t)1<<10)))
					return false;
			}
			return true;
		}
		bool CanAllocatePages(void* _base, size_t nPages, PageMap* pageMap)
		{
			// The reason a loop with PagesAllocated((void*)addr, 1, pageMap) is used and not return !PagesAllocated(_base, nPages, pageMap); is because
			// if one single page is unallocated, PagesAllocated would return false, even though preceding pages are allocated. So it would be like
			// if (!page1.present || !page2.present ...) return true, but we want if(!page1.present && !page2.present && ...) return true;
			
			uintptr_t base = (uintptr_t)_base & ~0xfff;
			for (uintptr_t addr = base; addr != (base + nPages * 4096); addr += 4096)
			{
				if (PagesAllocated((void*)addr, 1, pageMap))
					return false;
			}
			return true;
		}
		void freePagingStructures(uintptr_t* _pageMap, uintptr_t _pageMapPhys, obos::memory::PageMap* pageMap, uintptr_t addr)
		{
			if (utils::memcmp(_pageMap, (uint32_t)0, 4096))
			{
				freePhysicalPage(_pageMapPhys);
				uintptr_t _pageMap2Phys = (uintptr_t)pageMap->getL3PageMapEntryAt(addr) & g_physAddrMask;
				uintptr_t flags = saveFlagsAndCLI();
				uintptr_t* _pageMap2 = mapPageTable((uintptr_t*)_pageMap2Phys);
				_pageMap2[PageMap::addressToIndex(addr, 1)] = 0;
				if (utils::memcmp(_pageMap2, (uint32_t)0, 4096))
				{
					freePhysicalPage((uintptr_t)_pageMap2Phys);
					uintptr_t _pageMap3Phys = (uintptr_t)pageMap->getL4PageMapEntryAt(addr) & g_physAddrMask;
					uintptr_t* _pageMap3 = mapPageTable(reinterpret_cast<uintptr_t*>(_pageMap3Phys));
					_pageMap3[PageMap::addressToIndex(addr, 2)] = 0;
					if (utils::memcmp(_pageMap3, (uint32_t)0, 4096))
					{
						uintptr_t* _pageMap4 = mapPageTable(pageMap->getPageMap());
						_pageMap4[PageMap::addressToIndex(addr, 3)] = 0;
						freePhysicalPage((uintptr_t)_pageMap3Phys);
					}
				}
				restorePreviousInterruptStatus(flags);
			}
		}

		pageMapTuple GetPageMapFromProcess(process::Process* proc)
		{
			PageMap* pageMap = nullptr;
			bool isUserProcess = false;
			if (proc)
			{
				// The slab allocator allocates in a different range as liballoc.
				//OBOS_ASSERTP((uintptr_t)proc > 0xfffffffff0000000, "");
				isUserProcess = proc->isUsermode;
				pageMap = (PageMap*)proc->context.cr3;
			}
			else
			{
				thread::cpu_local* cpuLocalPtr = thread::GetCurrentCpuLocalPtr();
				if (cpuLocalPtr)
				{
					auto currentThread = cpuLocalPtr->currentThread;
					if (currentThread)
					{
						process::Process* process = (process::Process*)currentThread->owner;
						if (process)
						{
							isUserProcess = process->isUsermode;
							pageMap = (PageMap*)process->context.cr3;
						}
						else
							pageMap = getCurrentPageMap();
					}
					else
						pageMap = getCurrentPageMap();
				}
				else
					pageMap = getCurrentPageMap();
			}
			return { pageMap, isUserProcess };
		}

		void IteratePages(
			uintptr_t* headPM,
			uintptr_t* currentPM,
			uint8_t level,
			uintptr_t start,
			uintptr_t end,
			bool(*callback)(uintptr_t userdata, uintptr_t* pm, uintptr_t virt, uintptr_t entry),
			uintptr_t userdata,
			uintptr_t* indices // must be an array of at least 4 entries (anything over is ignored).
		)
		{
			size_t index = 0;
			size_t endIndex = 512;
			if (level == 3)
			{
				index = PageMap::addressToIndex(start, level);
				endIndex = PageMap::addressToIndex(end, level) + 1;
			}
			if (currentPM)
			{
				for (indices[level] = index; indices[level] < endIndex; indices[level]++)
				{
					uintptr_t entry = (mapPageTable(currentPM)[indices[level]]);
					bool isPresent = entry & 1;
					if (level == 0)
						isPresent = isPresent || (entry & ((uintptr_t)1<<10));
					if (!isPresent)
						continue;
					bool isHugePage = ((level == 2 || level == 1) && (entry & ((uintptr_t)1 << 7)));
					if (level == 0 || isHugePage)
					{
						uintptr_t virt = (indices[3] >= 0x100) ? 0xffff'8000'0000'0000 : 0 /* respect the canonical hole */;
						virt |= (indices[3] << 39);
						virt |= (indices[2] << 30);
						if (level != 2)
						{
							virt |= (indices[1] << 21);
							if (level != 1)
								virt |= (indices[0] << 12);
						}
						if (!callback(userdata, headPM, virt, entry))
						{
							// Set all the indices to 512 to stop iteration.
							indices[0] = indices[1] = indices[2] = indices[3] = 512;
							return;
						}
						continue;
					}
					IteratePages(headPM, (uintptr_t*)(entry & g_physAddrMask), level - 1, start, end, callback, userdata, indices);
				}
			}
		}
	}
}