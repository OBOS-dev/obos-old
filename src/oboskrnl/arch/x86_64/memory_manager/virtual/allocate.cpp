/*
	arch/x86_64/memory_manager/virtual/allocate.h

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>

#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <allocators/vmm/vmm.h>
#include <allocators/vmm/arch.h>

#include <multitasking/process/process.h>

#include <multitasking/cpu_local.h>

#include <x86_64-utils/asm.h>

#include <memory_manipulation.h>
#include <klog.h>

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
		static uintptr_t* allocatePagingStructures(uintptr_t address, PageMap* pageMap, uintptr_t flags)
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
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)pageMap->getL4PageMapEntryAt(address) & 0xFFFFFFFFFF000));
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
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)pageMap->getL4PageMapEntryAt(address) & 0xFFFFFFFFFF000));
				_pageMap[PageMap::addressToIndex(address, 2)] = entry;
				restorePreviousInterruptStatus(_flags);
			}
			if (!pageMap->getL2PageMapEntryAt(address))
			{
				uintptr_t entry = allocatePhysicalPage();
				uintptr_t _flags = saveFlagsAndCLI();
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)pageMap->getL3PageMapEntryAt(address) & 0xFFFFFFFFFF000));
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
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)pageMap->getL3PageMapEntryAt(address) & 0xFFFFFFFFFF000));
				_pageMap[PageMap::addressToIndex(address, 1)] = entry;
				restorePreviousInterruptStatus(_flags);
			}
			uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)pageMap->getL2PageMapEntryAt(address) & 0xFFFFFFFFFF000));
			return _pageMap;
		}

		static bool PagesAllocated(void* _base, size_t nPages, PageMap* pageMap)
		{
			uintptr_t base = (uintptr_t)_base & ~0xfff;
			for (uintptr_t addr = base; addr != (base + nPages * 4096); addr += 4096)
			{
				if (!pageMap->getL4PageMapEntryAt(addr))
					return false;
				if (!pageMap->getL3PageMapEntryAt(addr))
					return false;
				if (!pageMap->getL2PageMapEntryAt(addr))
					return false;
				if (!pageMap->getL1PageMapEntryAt(addr))
					return false;
			}
			return true;
		}
		static bool CanAllocatePages(void* _base, size_t nPages, PageMap* pageMap)
		{
			return !PagesAllocated(_base, nPages, pageMap);
		}
		static uintptr_t EncodeProtectionFlags(uintptr_t entry)
		{
			entry &= (~0xFFFFFFFFFF000);
			uintptr_t ret = 0;
			if (entry & 1)
				ret |= PROT_IS_PRESENT;
			else
				return 0;
			if (!(entry & ((uintptr_t)1 << 1)))
				ret |= PROT_READ_ONLY;
			if (entry & ((uintptr_t)1 << 4))
				ret |= PROT_DISABLE_CACHE;
			if (entry & ((uintptr_t)1 << 2))
				ret |= PROT_USER_MODE_ACCESS;
			if (!(entry & ((uintptr_t)1 << 63)))
				ret |= PROT_CAN_EXECUTE;
			return ret;
		}
		static void freePagingStructures(uintptr_t* _pageMap, uintptr_t _pageMapPhys, obos::memory::PageMap* pageMap, uintptr_t addr)
		{
			if (utils::memcmp(_pageMap, (uint32_t)0, 4096))
			{
				freePhysicalPage(_pageMapPhys);
				uintptr_t _pageMap2Phys = (uintptr_t)pageMap->getL3PageMapEntryAt(addr) & 0xFFFFFFFFFF000;
				uintptr_t flags = saveFlagsAndCLI();
				uintptr_t* _pageMap2 = mapPageTable((uintptr_t*)_pageMap2Phys);
				_pageMap2[PageMap::addressToIndex(addr, 1)] = 0;
				if (utils::memcmp(_pageMap2, (uint32_t)0, 4096))
				{
					freePhysicalPage((uintptr_t)_pageMap2Phys);
					uintptr_t _pageMap3Phys = (uintptr_t)pageMap->getL4PageMapEntryAt(addr) & 0xFFFFFFFFFF000;
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

		struct pageMapTuple { PageMap* pageMap; bool isUserMode; };
		static pageMapTuple GetPageMapFromProcess(process::Process* proc)
		{
			PageMap* pageMap = nullptr;
			bool isUserProcess = false;
			if (proc)
			{
				OBOS_ASSERTP((uintptr_t)proc > 0xfffffffff0000000, "");
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
		void* _Impl_FindUsableAddress(process::Process* proc, size_t nPages)
		{
			constexpr uintptr_t USER_BASE    = 0x0000000000001000;
			constexpr uintptr_t USER_LIMIT   = 0x00007fffffffffff;
			constexpr uintptr_t KERNEL_BASE  = 0xffffffff00000000;
			constexpr uintptr_t KERNEL_LIMIT = 0xffffffffffffd000;
			pageMapTuple tuple = GetPageMapFromProcess(proc);
			bool& isUserProcess = tuple.isUserMode;
			PageMap*& pageMap = tuple.pageMap;
			const uintptr_t base = isUserProcess ? USER_BASE : KERNEL_BASE;
			const uintptr_t limit = isUserProcess ? USER_LIMIT : KERNEL_LIMIT;
			uintptr_t addr = base;
			for (; addr < limit; addr += 0x1000)
				if (CanAllocatePages((void*)addr, nPages, pageMap))
					break;
			if (addr == limit)
				return nullptr;
			return (void*)addr;
		}
		void* _Impl_ProcVirtualAlloc(process::Process* proc, void* _base, size_t nPages, uintptr_t protFlags, uint32_t* status)
		{
			if (!_Impl_IsValidAddress(_base))
			{
				*status = VALLOC_INVALID_PARAMETER;
				return nullptr;
			}
			pageMapTuple tuple = GetPageMapFromProcess(proc);
			bool &isUserProcess = tuple.isUserMode;
			PageMap*& pageMap = tuple.pageMap;
			if ((uintptr_t)_base > 0xffff800000000000 && isUserProcess)
			{
				*status = VALLOC_ACCESS_DENIED;
				return nullptr;
			}
			if (!CanAllocatePages(_base, nPages, pageMap))
			{
				*status = VALLOC_BASE_ADDRESS_USED;
				return nullptr;
			}
			protFlags &= PROT_ALL_BITS_SET;
			uintptr_t cpuFlags = DecodeProtectionFlags(protFlags) | 1;
			uintptr_t demandPagingEntry = ((uintptr_t)1 << 9) | (protFlags << 52) | 1 | ((uintptr_t)1<<63); // Bit 9: COW, Bits 52-58: Prot Flags.
			bool demandPagingEnabled = !(protFlags & PROT_NO_COW_ON_ALLOCATE);
			if(demandPagingEnabled)
				cpuFlags = 1 | ((uintptr_t)1<<63);
			uintptr_t base = (uintptr_t)_base;
			for (uintptr_t addr = base; addr < (base + nPages * 0x1000); addr += 0x1000)
			{
				uintptr_t entry = 0;
				if (demandPagingEnabled)
					entry = demandPagingEntry;
				else
					entry = allocatePhysicalPage() | cpuFlags | 1;
				invlpg(addr);
				uintptr_t* _pageMap = allocatePagingStructures(addr, pageMap, cpuFlags);
				_pageMap[PageMap::addressToIndex(addr, 0)] = entry;
				invlpg(addr);
			}
			*status = VALLOC_SUCCESS;
			return (void*)base;
		}
		void _Impl_ProcVirtualFree(process::Process* proc, void* _base, size_t nPages, uint32_t* status)
		{
			if (!_Impl_IsValidAddress(_base))
			{
				*status = VFREE_INVALID_PARAMETER;
				return;
			}
			pageMapTuple tuple = GetPageMapFromProcess(proc);
			bool& isUserProcess = tuple.isUserMode;
			PageMap*& pageMap = tuple.pageMap;
			if ((uintptr_t)_base > 0xffff800000000000 && isUserProcess)
			{
				*status = VFREE_ACCESS_DENIED;
				return;
			}
			if (CanAllocatePages(_base, nPages, pageMap))
			{
				*status = VFREE_MEMORY_UNMAPPED;
				return;
			}
			*status = VFREE_SUCCESS;
			uintptr_t base = (uintptr_t)_base & (~0xfff);
			uintptr_t eflags = saveFlagsAndCLI();
			for (uintptr_t addr = base; addr != (base + nPages * 4096); addr += 4096)
			{
				uintptr_t _pageMapPhys = (uintptr_t)pageMap->getL2PageMapEntryAt(addr) & 0xFFFFFFFFFF000;
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>(_pageMapPhys));
				uintptr_t entry = _pageMap[PageMap::addressToIndex(addr, 0)];
				if (!(entry & ((uintptr_t)1 << 9)))
					freePhysicalPage(entry & 0xFFFFFFFFFF000);
				_pageMap = mapPageTable(reinterpret_cast<uintptr_t*>(_pageMapPhys));
				_pageMap[PageMap::addressToIndex(addr, 0)] = 0;
				freePagingStructures(_pageMap, _pageMapPhys, pageMap, addr);
				invlpg(addr);
			}
			restorePreviousInterruptStatus(eflags);
			*status = VFREE_SUCCESS;
			return;
		}
		void _Impl_ProcVirtualProtect(process::Process* proc, void* _base, size_t nPages, uintptr_t _flags, uint32_t* status)
		{
			if (!_Impl_IsValidAddress(_base))
			{
				*status = VPROTECT_INVALID_PARAMETER;
				return;
			}
			pageMapTuple tuple = GetPageMapFromProcess(proc);
			bool& isUserProcess = tuple.isUserMode;
			PageMap*& pageMap = tuple.pageMap;
			if ((uintptr_t)_base > 0xffff800000000000 && isUserProcess)
			{
				*status = VPROTECT_ACCESS_DENIED;
				return;
			}
			if (CanAllocatePages(_base, nPages, pageMap))
			{
				*status = VPROTECT_MEMORY_UNMAPPED;
				return;
			}
			*status = VPROTECT_SUCCESS;
			_flags &= PROT_ALL_BITS_SET;
			uintptr_t base = (uintptr_t)_base & (~0xfff);
			for (uintptr_t addr = base; addr != (base + nPages * 4096); addr += 4096)
			{
				uintptr_t _pageMapPhys = (uintptr_t)pageMap->getL2PageMapEntryAt(addr) & 0xFFFFFFFFFF000;
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>(_pageMapPhys));
				uintptr_t entry = _pageMap[PageMap::addressToIndex(addr, 0)];
				uintptr_t newEntry = 0;
				if (entry & ((uintptr_t)1 << 9))
					newEntry = 1 | (_flags << 52) | ((uintptr_t)1 << 9);
				else
					newEntry = (entry & 0xFFFFFFFFFF000) | DecodeProtectionFlags(_flags) | 1;
				_pageMap = allocatePagingStructures(addr, pageMap, DecodeProtectionFlags(_flags) | 1);
				_pageMap[PageMap::addressToIndex(addr, 0)] = newEntry;
				invlpg(addr);
			}
		}
		void _Impl_ProcVirtualGetProtection(process::Process* proc, void* _base, size_t nPages, uintptr_t* flags, uint32_t* status)
		{
			if (!_Impl_IsValidAddress(_base))
			{
				*status = VGETPROT_INVALID_PARAMETER;
				return;
			}
			pageMapTuple tuple = GetPageMapFromProcess(proc);
			bool& isUserProcess = tuple.isUserMode;
			PageMap*& pageMap = tuple.pageMap;
			if ((uintptr_t)_base > 0xffff800000000000 && isUserProcess)
			{
				*status = VGETPROT_ACCESS_DENIED;
				return;
			}
			*status = VGETPROT_SUCCESS;
			uintptr_t base = (uintptr_t)_base & (~0xfff);
			for (uintptr_t addr = base, i = 0; addr != (base + nPages * 4096); addr += 4096, i++)
			{
				if (!PagesAllocated((void*)addr, 1, pageMap))
				{
					flags[i] = 0;
					continue;
				}
				uintptr_t entry = (uintptr_t)pageMap->getL1PageMapEntryAt(addr);
				flags[i] = EncodeProtectionFlags(entry);
			}
		}

		size_t _Impl_GetPageSize()
		{
			return 0x1000;
		}
		bool _Impl_IsValidAddress(void* addr)
		{
			uintptr_t addr47_63 = (uintptr_t)addr >> 47;
			return addr47_63 == 0 || addr47_63 == 0x1ffff;
		}
		
		void* MapPhysicalAddress(PageMap* pageMap, uintptr_t phys, void* to, uintptr_t cpuFlags)
		{
			uintptr_t* pageTable = allocatePagingStructures((uintptr_t)to, pageMap, cpuFlags);
			pageTable[PageMap::addressToIndex((uintptr_t)to, 0)] = phys | cpuFlags;
			invlpg((uintptr_t)to);
			return to;
		}
		void* MapEntry(PageMap* pageMap, uintptr_t entry, void* to)
		{
			uintptr_t* pageTable = allocatePagingStructures((uintptr_t)to, pageMap, entry & 0x800000000000003F);
			pageTable[PageMap::addressToIndex((uintptr_t)to, 0)] = entry;
			invlpg((uintptr_t)to);
			return to;
		}
		void UnmapAddress(PageMap* pageMap, void* _addr)
		{
			uintptr_t addr = (uintptr_t)_addr;
			uintptr_t eflags = saveFlagsAndCLI();
			uintptr_t _pageMapPhys = (uintptr_t)pageMap->getL2PageMapEntryAt(addr) & 0xFFFFFFFFFF000;
			uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>(_pageMapPhys));
			_pageMap = mapPageTable(reinterpret_cast<uintptr_t*>(_pageMapPhys));
			_pageMap[PageMap::addressToIndex(addr, 0)] = 0;
			freePagingStructures(_pageMap, _pageMapPhys, pageMap, addr);
			invlpg(addr);
			restorePreviousInterruptStatus(eflags);
		}
		void* _Impl_Memcpy(process::Process* proc, void* remoteDest, void* localSrc, size_t size, uint32_t* status)
		{
			pageMapTuple tuple = GetPageMapFromProcess(proc);
			bool& isUserProcess = tuple.isUserMode;
			PageMap*& pageMap = tuple.pageMap;
			size_t nPagesDest = size / 4096 + ((((uintptr_t)remoteDest + size) & ~0xfff) > ((uintptr_t)remoteDest));
			size_t nPagesSrc  = size / 4096 + ((((uintptr_t)localSrc   + size) & ~0xfff) > ((uintptr_t)localSrc));
			if (size < 4096)
			{
				nPagesDest++;
				nPagesSrc++;
			}
			// We're going to do this one page at a time.
			uint32_t _status = 0;
			uintptr_t flags = 0;
			for (uintptr_t dest = (uintptr_t)remoteDest & ~0xfff; dest < (((uintptr_t)remoteDest & ~0xfff) + nPagesDest * 4096); dest += 4096)
			{
				_Impl_ProcVirtualGetProtection(proc, (void*)dest, 1, &flags, &_status);
				if (_status == VGETPROT_ACCESS_DENIED)
				{
					*status = MEMCPY_DESTINATION_PFAULT;
					return nullptr;
				}
				if (!(flags & PROT_IS_PRESENT) || !(flags & PROT_READ_ONLY) || (!(flags & PROT_USER_MODE_ACCESS) && isUserProcess))
				{
					*status = MEMCPY_DESTINATION_PFAULT;
					return nullptr;
				}
			}
			if (!PagesAllocated(localSrc, nPagesSrc, pageMap))
			{
				*status = MEMCPY_SOURCE_PFAULT;
				return nullptr;
			}
			// Copy the data.
			void* baseDest, *realDest = baseDest = _Impl_FindUsableAddress(nullptr, nPagesDest);
			uintptr_t eflags = saveFlagsAndCLI();
			for (uintptr_t dest = (uintptr_t)remoteDest & ~0xfff; dest < (((uintptr_t)remoteDest & ~0xfff) + nPagesDest * 4096); dest += 4096)
			{
				uintptr_t* pageTable = mapPageTable((uintptr_t*)((uintptr_t)pageMap->getL2PageMapEntryAt(dest) & 0xFFFFFFFFFF000));
				uintptr_t& pageTableEntry = pageTable[PageMap::addressToIndex(dest, 0)];
				uintptr_t _pageTableEntry = pageTable[PageMap::addressToIndex(dest, 0)];
				if (pageTableEntry & ((uintptr_t)1 << 9))
				{
					// We must allocate the page.
					uintptr_t entry = 0;
					entry |= allocatePhysicalPage();
					void* entry_addr = _Impl_FindUsableAddress(nullptr, 1);
					MapPhysicalAddress(getCurrentPageMap(), _pageTableEntry & 0xFFFFFFFFFF000, entry_addr, ((uintptr_t)1) | ((uintptr_t)2) | ((uintptr_t)1 << 63));
					utils::memcpy(memory::mapPageTable((uintptr_t*)entry), entry_addr, 4096);
					UnmapAddress(getCurrentPageMap(), entry_addr);
					entry |= DecodeProtectionFlags(_pageTableEntry >> 52) | 1;
					pageTable = mapPageTable((uintptr_t*)((uintptr_t)pageMap->getL2PageMapEntryAt(dest) & 0xFFFFFFFFFF000));
					pageTable[PageMap::addressToIndex(dest, 0)] = entry;
				}
				pageTable = mapPageTable((uintptr_t*)((uintptr_t)pageMap->getL2PageMapEntryAt(dest) & 0xFFFFFFFFFF000));
				MapPhysicalAddress(getCurrentPageMap(), pageTableEntry & 0xFFFFFFFFFF000, realDest, ((uintptr_t)1) | ((uintptr_t)2) | ((uintptr_t)1 << 63));
				realDest = (void*)((uintptr_t)realDest + 4096);
			}
			restorePreviousInterruptStatus(eflags);
			baseDest = (void*)((uintptr_t)baseDest + (((uintptr_t)remoteDest) & 0xfff));
			utils::memcpy(baseDest, localSrc, size);
			baseDest = (void*)((uintptr_t)baseDest & ~0xfff);
			for (uintptr_t _base = (uintptr_t)baseDest; _base < (((uintptr_t)baseDest) + nPagesDest * 4096); _base += 4096)
				UnmapAddress(getCurrentPageMap(), (void*)_base);
			*status = MEMCPY_SUCCESS;
			return remoteDest;
		}
	}
}