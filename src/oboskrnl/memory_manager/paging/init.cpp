/*
	oboskrnl/memory_manager/paging/init.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/paging/init.h>
#include <memory_manager/physical.h>

#include <new>

#define indexToAddress(index, pageTableIndex) ((UINTPTR_T)(((index) << 22) + ((pageTableIndex) << 12)))
#define inRange(val, rStart, rEnd) (((UINTPTR_T)(val)) >= ((UINTPTR_T)(rStart)) && val <= ((UINTPTR_T)(rEnd)))

extern "C" UINTPTR_T boot_page_directory1;

namespace obos
{
	namespace memory
	{
		bool g_enabledPaging = false;

		PageDirectory::PageDirectory()
		{
			init();
		}
		PageDirectory::PageDirectory(UINTPTR_T* array)
		{
			m_array = array;
			m_initialized = true;
		}

		void PageDirectory::init()
		{
			if (m_initialized)
				destroy();
			m_array = (UINTPTR_T*)kalloc_physicalPages(1);
			m_owns = m_initialized = true;
		}

		void PageDirectory::switchToThis()
		{
			if (!m_initialized)
				return;
			switchToThisAsm((UINTPTR_T)m_array & 0xFFFFF000);
		}
		extern UINTPTR_T* kmap_pageTable(PVOID physicalAddress);

		UINTPTR_T* PageDirectory::getPageTableAddress(UINT16_T pageTable)
		{
			if (pageTable > 1023 || !m_initialized)
				return nullptr;
			//return (g_enabledPaging ? kmap_pageTable(m_array) : m_array) + pageTable;
			return m_array + pageTable;
		}
		UINTPTR_T* PageDirectory::getPageTable(UINT16_T pageTable)
		{
			if (pageTable > 1023 || !m_initialized)
				return nullptr;
			//return (UINTPTR_T*)((g_enabledPaging ? kmap_pageTable(m_array) : m_array)[pageTable] & 0xFFFFF000);
			return (UINTPTR_T*)(m_array[pageTable] & 0xFFFFF000);
		}

		void PageDirectory::destroy()
		{
			if (m_initialized && m_owns)
			{
				// the next two lines are fine, look in kfree_physicalPages before changing them.
				for (int i = 0; i < 1024; i++)
					kfree_physicalPages((PVOID)m_array[i], 1);
				kfree_physicalPages(m_array, 1);
			}
		}

		PageDirectory::~PageDirectory()
		{
			destroy();
		}

		UINT16_T PageDirectory::addressToIndex(UINTPTR_T base)
		{
			return base >> 22;
		}
		UINT16_T PageDirectory::addressToPageTableIndex(UINTPTR_T base)
		{
			return (base >> 12) % 1024;
		}

		PageDirectory* g_pageDirectory;
		
		extern UINTPTR_T s_lastPageTable[1024] attribute(aligned(4096));

		void InitializePaging()
		{
			/*auto constructor = (void(*)(obos::memory::PageDirectory* _this))GET_FUNC_ADDR(&_ZN4obos6memory13PageDirectoryC1Ev);
			constructor(g_pageDirectory);*/
			if (g_enabledPaging)
				return;
			memory::g_pageDirectory = new (g_pageDirectory) memory::PageDirectory{ &boot_page_directory1 };
			*memory::g_pageDirectory->getPageTableAddress(1023) = (((UINTPTR_T)&s_lastPageTable) - 0xC0000000) | 7;
			g_enabledPaging = true;
		}

		void tlbFlush(UINT32_T addr)
		{
			asm volatile("invlpg (%0)" ::"b"(addr) : "memory");
		}
	}
}