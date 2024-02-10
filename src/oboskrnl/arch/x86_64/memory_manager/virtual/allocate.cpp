/*
	arch/x86_64/memory_manager/virtual/allocate.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>
#include <arch/x86_64/memory_manager/virtual/internal.h>

#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <allocators/vmm/vmm.h>
#include <allocators/vmm/arch.h>

#include <x86_64-utils/asm.h>

namespace obos
{
	namespace memory
	{
		void IteratePages(
			uintptr_t* headPM,
			uintptr_t* currentPM,
			uint8_t level,
			uintptr_t start,
			uintptr_t end,
			bool(*callback)(uintptr_t userdata, uintptr_t* pm, uintptr_t virt, uintptr_t entry),
			uintptr_t userdata,
			uintptr_t* indices // must be an array of at least 4 entries (anything over is ignored).
		);
		
		static uintptr_t EncodeProtectionFlags(uintptr_t entry)
		{
			entry &= (~g_physAddrMask);
			uintptr_t ret = 0;
			if (entry & 1)
				ret |= PROT_IS_PRESENT;
			else
				return 0;
			if (entry & ((uintptr_t)1 << 9))
				return ret | ((entry >> 52) & 0b1111111);
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

		void* _Impl_FindUsableAddress(process::Process* proc, size_t nPages)
		{
			constexpr uintptr_t USER_BASE = 0x0000000000001000;
			constexpr uintptr_t USER_LIMIT = 0x00007fffffffffff;
			constexpr uintptr_t KERNEL_BASE = 0xffffffff00000000 - 0x1000;
			constexpr uintptr_t KERNEL_LIMIT = 0xffffffffffffc000;
			auto callBack =
				[](uintptr_t _userdata, uintptr_t*, uintptr_t virt, uintptr_t)->bool
				{
					uintptr_t* userdata = (uintptr_t*)_userdata;
					uintptr_t& base = userdata[0];
					uintptr_t& ret = userdata[2];
					uintptr_t& lastAddress = userdata[3];
					size_t& nPages = userdata[4];
					if (virt < base)
						return true;
					if (((virt - lastAddress) / 4096) >= nPages + 1)
					{
						ret = lastAddress /* if we set it to 'virt', we would be referring to an already mapped page */;
						return false;
					}
					lastAddress = virt;
					return true;
				};
			pageMapTuple tuple = GetPageMapFromProcess(proc);
			bool& isUserProcess = tuple.isUserMode;
			PageMap*& pageMap = tuple.pageMap;
			const uintptr_t base = isUserProcess ? USER_BASE : KERNEL_BASE;
			const uintptr_t limit = isUserProcess ? USER_LIMIT : KERNEL_LIMIT;
			uintptr_t indices[4] = {};
			uintptr_t userdata[] = {
				base /* base */, 
				limit /* limit */, 
				limit /* address found */,
				base/* last address */, 
				nPages /* number of pages that should be free */,
			};
			uintptr_t* pml4 = pageMap->getPageMap();
			IteratePages(pml4, pml4, 3, base, limit, callBack , (uintptr_t)&userdata, indices);
			if (userdata[2] == limit)
			{
				// Try again
				IteratePages(pml4, pml4, 3, base, limit, callBack, (uintptr_t)&userdata, indices);
				if (userdata[2] == limit)
					return nullptr;
			}
			return (void*)(userdata[2] + 0x1000);
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
				uintptr_t _pageMapPhys = (uintptr_t)pageMap->getL2PageMapEntryAt(addr) & g_physAddrMask;
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>(_pageMapPhys));
				uintptr_t entry = _pageMap[PageMap::addressToIndex(addr, 0)];
				if (!(entry & ((uintptr_t)1 << 9)) && !(entry & ((uintptr_t)1 << 10)))
					freePhysicalPage(entry & g_physAddrMask);
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
				uintptr_t _pageMapPhys = (uintptr_t)pageMap->getL2PageMapEntryAt(addr) & g_physAddrMask;
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>(_pageMapPhys));
				uintptr_t entry = _pageMap[PageMap::addressToIndex(addr, 0)];
				uintptr_t newEntry = 0;
				if (entry & ((uintptr_t)1 << 9))
					newEntry = 1 | (_flags << 52) | ((uintptr_t)1 << 9) | ((uintptr_t)1 << 63);
				else
					newEntry = (entry & g_physAddrMask) | DecodeProtectionFlags(_flags) | 1;
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
			uintptr_t _pageMapPhys = (uintptr_t)pageMap->getL2PageMapEntryAt(addr) & g_physAddrMask;
			uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>(_pageMapPhys));
			_pageMap = mapPageTable(reinterpret_cast<uintptr_t*>(_pageMapPhys));
			_pageMap[PageMap::addressToIndex(addr, 0)] = 0;
			freePagingStructures(_pageMap, _pageMapPhys, pageMap, addr);
			invlpg(addr);
			restorePreviousInterruptStatus(eflags);
		}
		void* _Impl_Memcpy(process::Process* proc, void* remoteDest, const void* localSrc, size_t size, uint32_t* status)
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
				if (!(flags & PROT_IS_PRESENT) && flags & PROT_READ_ONLY && (!(flags & PROT_USER_MODE_ACCESS) && isUserProcess))
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
				uintptr_t* pageTable = mapPageTable((uintptr_t*)((uintptr_t)pageMap->getL2PageMapEntryAt(dest) & g_physAddrMask));
				uintptr_t& pageTableEntry = pageTable[PageMap::addressToIndex(dest, 0)];
				uintptr_t _pageTableEntry = pageTable[PageMap::addressToIndex(dest, 0)];
				if (pageTableEntry & ((uintptr_t)1 << 9))
				{
					// We must allocate the page.
					uintptr_t entry = 0;
					entry |= allocatePhysicalPage();
					void* entry_addr = _Impl_FindUsableAddress(nullptr, 1);
					MapPhysicalAddress(getCurrentPageMap(), _pageTableEntry & g_physAddrMask, entry_addr, ((uintptr_t)1) | ((uintptr_t)2) | ((uintptr_t)1 << 63));
					utils::memcpy(memory::mapPageTable((uintptr_t*)entry), entry_addr, 4096);
					UnmapAddress(getCurrentPageMap(), entry_addr);
					entry |= DecodeProtectionFlags(_pageTableEntry >> 52) | 1;
					pageTable = mapPageTable((uintptr_t*)((uintptr_t)pageMap->getL2PageMapEntryAt(dest) & g_physAddrMask));
					pageTable[PageMap::addressToIndex(dest, 0)] = entry;
				}
				pageTable = mapPageTable((uintptr_t*)((uintptr_t)pageMap->getL2PageMapEntryAt(dest) & g_physAddrMask));
				MapPhysicalAddress(getCurrentPageMap(), pageTableEntry & g_physAddrMask, realDest, ((uintptr_t)1) | ((uintptr_t)2) | ((uintptr_t)1 << 63));
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
		bool _Impl_FreeUserProcessAddressSpace(process::Process *proc)
		{
			if (!proc)
				return false;
			// Free everything from nullptr-0x7FFFFFFFFFFF
			auto[_pageMap, isUserProcess] = GetPageMapFromProcess(proc);
			if (!isUserProcess)
				return false;
			uintptr_t* pml4 = _pageMap->getPageMap();
			uintptr_t start = 0, end = 0x7FFFFFFFFFFF;
			// Iterate over the page tables, going into them if present == true.
			uintptr_t indices[4];
			IteratePages(pml4, pml4, 3, start, end, 
			[](uintptr_t, uintptr_t *pm, uintptr_t virt, uintptr_t entry)
			{
				PageMap* _pm = (PageMap*)pm;
				uintptr_t _pml2Phys = (uintptr_t)_pm->getL2PageMapEntryAt(virt) & g_physAddrMask;
				if (!(entry & ((uintptr_t)1 << 9)))
					freePhysicalPage(entry & g_physAddrMask);
				mapPageTable((uintptr_t*)_pml2Phys)[PageMap::addressToIndex(virt, 0)] = 0;
				freePagingStructures(mapPageTable((uintptr_t*)_pml2Phys), _pml2Phys, _pm, virt);
				return true;
			}, 0, indices);
			return true;
		}
    }
}