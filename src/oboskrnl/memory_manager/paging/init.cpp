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
extern "C" char _stext;

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
			m_array = (UINTPTR_T*)kalloc_physicalPages(1);
			m_owns = m_initialized = true;
		}

		void PageDirectory::switchToThis()
		{
			if (!m_initialized)
				return;
			g_pageDirectory = this;
			switchToThisAsm((UINTPTR_T)m_array & 0xFFFFF000);
		}
		extern UINTPTR_T* kmap_pageTable(PVOID physicalAddress);

		UINTPTR_T* PageDirectory::getPageTableAddress(UINT16_T pageTable)
		{
			if (pageTable > 1023 || !m_initialized)
				return nullptr;
			UINTPTR_T* array = nullptr;
			if (m_array == &boot_page_directory1)
				array = reinterpret_cast<UINTPTR_T*>(reinterpret_cast<UINTPTR_T>(m_array) + 0xC0000000);
			else
				array = kmap_pageTable(m_array);
			return array + pageTable;
		}
		UINTPTR_T* PageDirectory::getPageTable(UINT16_T pageTable)
		{
			if (pageTable > 1023 || !m_initialized)
				return nullptr;
			UINTPTR_T* array = nullptr;
			if (m_array == &boot_page_directory1)
				array = reinterpret_cast<UINTPTR_T*>(reinterpret_cast<UINTPTR_T>(m_array) + 0xC0000000);
			else
				array = kmap_pageTable(m_array);
			return (UINTPTR_T*)(array[pageTable] & 0xFFFFF000);
		}

		PageDirectory::~PageDirectory()
		{
			if (m_owns)
				kfree_physicalPages(m_array, 1);
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
			//*memory::g_pageDirectory->getPageTableAddress(0) = 0;
			*memory::g_pageDirectory->getPageTableAddress(1023) = (((UINTPTR_T)&s_lastPageTable) - 0xC0000000) | 7;
			g_enabledPaging = true;
			UINT32_T* pageTable = kmap_pageTable(memory::g_pageDirectory->getPageTable(768));
			for (int i = PageDirectory::addressToPageTableIndex((reinterpret_cast<UINTPTR_T>(&_stext) >> 12) << 12);
				i < PageDirectory::addressToPageTableIndex(((reinterpret_cast<UINTPTR_T>(&__erodata) >> 12) + 1) << 12); i++)
			{
				UINTPTR_T addr = indexToAddress(768, i);
				(addr = addr);
				pageTable[i] &= ~2;
			}
		}

		void tlbFlush(UINTPTR_T addr)
		{
			asm volatile("invlpg (%0)" ::"b"(addr) : "memory");
		}
	}
}