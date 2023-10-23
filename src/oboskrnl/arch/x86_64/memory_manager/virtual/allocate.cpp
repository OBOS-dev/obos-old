/*
	arch/x86_64/memory_manager/virtual/allocate.h

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <memory_manipulation.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>
#include <arch/x86_64/memory_manager/virtual/allocate.h>

#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <x86_64-utils/asm.h>

namespace obos
{
	namespace memory
	{
		static uintptr_t* allocatePagingStructures(uintptr_t address)
		{
			PageMap* pageMap = getCurrentPageMap();
			uintptr_t flags = 3 | ((uintptr_t)1 << CPUSupportsExecuteDisable());

			if (!pageMap->getL4PageMapEntryAt(address))
			{
				uintptr_t entry = allocatePhysicalPage();
				uintptr_t* _pageMap = mapPageTable(pageMap->getPageMap());
				_pageMap[PageMap::addressToIndex(address, 3)] = entry | flags;
				utils::memzero(mapPageTable((uintptr_t*)entry), 4096);
			}
			if (!pageMap->getL3PageMapEntryAt(address))
			{
				uintptr_t entry = allocatePhysicalPage();
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)pageMap->getL4PageMapEntryAt(address) & 0xFFFFFFFFFF000));
				_pageMap[PageMap::addressToIndex(address, 2)] = entry | flags;
				utils::memzero(mapPageTable((uintptr_t*)entry), 4096);
			}
			if (!pageMap->getL2PageMapEntryAt(address))
			{
				uintptr_t entry = allocatePhysicalPage();
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)pageMap->getL3PageMapEntryAt(address) & 0xFFFFFFFFFF000));
				_pageMap[PageMap::addressToIndex(address, 1)] = entry | flags;
				utils::memzero(mapPageTable((uintptr_t*)entry), 4096);
			}
			uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)pageMap->getL2PageMapEntryAt(address) & 0xFFFFFFFFFF000));
			return _pageMap;
		}

		static bool PagesAllocated(void* _base, size_t nPages)
		{
			PageMap* pageMap = getCurrentPageMap();
			uintptr_t base = (uintptr_t)_base;
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
		static bool CanAllocatePages(void* _base, size_t nPages)
		{
			uintptr_t base = (uintptr_t)_base;
			for (uintptr_t addr = base; addr != (base + nPages * 4096); addr += 4096)
			{
				if (PagesAllocated((void*)base, 1))
					return false;
			}
			return true;
		}

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

		void* VirtualAlloc(void* _base, size_t nPages, uintptr_t _flags)
		{
			if(_base != nullptr)
				if (!CanAllocatePages(_base, nPages))
					return nullptr;
			uintptr_t base = (uintptr_t)_base & (~0xfff);
			if (!base)
			{
				base = 0x1000;
				while (1)
				{
					for (; PagesAllocated(reinterpret_cast<void*>(base), 1); base += 4096);
					if (!PagesAllocated(reinterpret_cast<void*>(base), nPages))
						break;
				}
				if (!base)
					return nullptr;
			}
			_flags &= PROT_ALL_BITS_SET;
			uintptr_t flags = DecodeProtectionFlags(_flags) | 1;
			bool cowPages = !(_flags & PROT_NO_COW_ON_ALLOCATE);
			_flags &= ~PROT_NO_COW_ON_ALLOCATE;
			for (uintptr_t addr = base; addr != (base + nPages * 4096); addr += 4096)
			{
				uintptr_t entry = 1;
				if (cowPages)
				{
					entry |= ((uintptr_t)1 << 9);
					entry |= reinterpret_cast<uintptr_t>(_flags) << 52;
				}
				else
				{
					entry |= flags;
					entry |= allocatePhysicalPage();
				}
				MapVirtualPageToEntry((void*)addr, entry);
			}
			return reinterpret_cast<void*>(base);
		}
		
		static void freePagingStructures(uintptr_t* _pageMap, uintptr_t _pageMapPhys, obos::memory::PageMap* pageMap, uintptr_t addr)
		{
			if (utils::memcmp(_pageMap, (uint32_t)0, 4096))
			{
				freePhysicalPage(_pageMapPhys);
				uintptr_t _pageMap2Phys = (uintptr_t)pageMap->getL3PageMapEntryAt(addr) & 0xFFFFFFFFFF000;
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
			}
		}

		bool VirtualFree(void* _base, size_t nPages)
		{
			if (!_base)
				return false;
			if (CanAllocatePages(_base, nPages))
				return false;
			uintptr_t base = (uintptr_t)_base & (~0xfff);
			PageMap* pageMap = getCurrentPageMap();
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
			return true;
		}
		
		bool VirtualProtect(void* _base, size_t nPages, uintptr_t _flags)
		{
			if (!_base)
				return false;
			if (CanAllocatePages(_base, nPages))
				return false;
			uintptr_t base = (uintptr_t)_base & (~0xfff);
			PageMap* pageMap = getCurrentPageMap();
			_flags &= PROT_ALL_BITS_SET;
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
				_pageMap[PageMap::addressToIndex(addr, 0)] = newEntry;
				invlpg(addr);
			}
			return true;
		}

		static uintptr_t EncodeProtectionFlags(uintptr_t entry)
		{
			entry &= (~0xFFFFFFFFFF000);
			uintptr_t ret = 0;
			if (entry & 1)
				ret |= PROT_IS_PRESENT;
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
		
		bool VirtualGetProtection(void* _base, size_t nPages, uintptr_t* _flags)
		{
			if (!_base)
				return false;
			uintptr_t base = (uintptr_t)_base & (~0xfff);
			PageMap* pageMap = getCurrentPageMap();
			for (uintptr_t addr = base, i = 0; addr != (base + nPages * 4096); addr += 4096, i++)
			{
				if (!PagesAllocated((void*)addr, 1))
				{
					_flags[i] = 0;
					continue;
				}
				uintptr_t _pageMapPhys = (uintptr_t)pageMap->getL2PageMapEntryAt(addr) & 0xFFFFFFFFFF000;
				uintptr_t* _pageMap = mapPageTable(reinterpret_cast<uintptr_t*>(_pageMapPhys));
				uintptr_t entry = _pageMap[PageMap::addressToIndex(addr, 0)];
				_flags[i] = EncodeProtectionFlags(entry);
			}
			return true;
		}

		void MapVirtualPageToPhysical(void* virt, void* phys, uintptr_t cpuFlags)
		{
			MapVirtualPageToEntry(virt, (uintptr_t)phys | cpuFlags);
		}
		void MapVirtualPageToEntry(void* _virt, uintptr_t entry)
		{
			uintptr_t virt = (uintptr_t)_virt;
			uintptr_t* pageTable = allocatePagingStructures(virt);
			pageTable[PageMap::addressToIndex(virt, 0)] = entry;
			invlpg(virt);
		}

	}
}