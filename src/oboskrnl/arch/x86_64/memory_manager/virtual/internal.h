/*
	 oboskrnl/arch/x86_64/memory_manager/virtual/internal.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>

#include <multitasking/process/process.h>

namespace obos
{
	namespace memory
	{
		struct pageMapTuple { PageMap* pageMap; bool isUserMode; };
		pageMapTuple GetPageMapFromProcess(process::Process* proc);
		bool CanAllocatePages(void* _base, size_t nPages, PageMap* pageMap);
		bool PagesAllocated(const void* _base, size_t nPages, PageMap* pageMap);
		uintptr_t DecodeProtectionFlags(uintptr_t _flags);

		uintptr_t* allocatePagingStructures(uintptr_t address, PageMap* pageMap, uintptr_t flags);
		void freePagingStructures(uintptr_t* _pageMap, uintptr_t _pageMapPhys, obos::memory::PageMap* pageMap, uintptr_t addr);

		// This includes non-committed pages (ie. pages that have bit 10 of the pte set, but not bit 0).
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

		void* MapPhysicalAddress(PageMap* pageMap, uintptr_t phys, void* to, uintptr_t cpuFlags);
		void* MapEntry(PageMap* pageMap, uintptr_t entry, void* to);
		void UnmapAddress(PageMap* pageMap, void* _addr);
	}
}