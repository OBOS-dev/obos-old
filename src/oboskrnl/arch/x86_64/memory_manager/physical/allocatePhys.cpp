/*
	oboskrnl/arch/x86_64/memory_manager/allocate.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <limine.h>

#include <int.h>
#include <klog.h>
#include <atomic.h>
#include <memory_manipulation.h>

#include <x86_64-utils/asm.h>

#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>

#include <multitasking/locks/mutex.h>

#include <new>

namespace obos
{
	static volatile limine_memmap_request mmap_request = {
		.id = LIMINE_MEMMAP_REQUEST,
		.revision = 1
	};
	namespace memory
	{
		struct MemoryNode
		{
			MemoryNode *next, *prev;
			size_t nPages;
		};
		MemoryNode *g_memoryHead, *g_memoryTail;
		size_t g_nMemoryNodes;
		locks::Mutex g_pmmLock;

#pragma GCC push_options
#pragma GCC optimize("O1")
		void InitializePhysicalMemoryManager()
		{
			new (&g_pmmLock) locks::Mutex{ false };
			for (size_t i = 0; i < mmap_request.response->entry_count; i++)
			{
				if (mmap_request.response->entries[i]->type != LIMINE_MEMMAP_USABLE)
					continue;
				uintptr_t base = mmap_request.response->entries[i]->base;
				if (base < 0x1000)
					base = 0x1000;
				if (base & 0xfff)
					base = (base + 0xfff) & 0xfff;
				size_t size = mmap_request.response->entries[i]->length & ~0xfff;
				g_nMemoryNodes++;
				MemoryNode* newNode = (MemoryNode*)mapPageTable((uintptr_t*)base);
				MemoryNode* newNodePhys = (MemoryNode*)base;
				if (g_memoryTail)
					((MemoryNode*)mapPageTable((uintptr_t*)g_memoryTail))->next = newNodePhys;
				if(!g_memoryHead)
					g_memoryHead = newNodePhys;
				newNode->prev = g_memoryTail;
				newNode->next = nullptr;
				newNode->nPages = size / 0x1000;
				g_memoryTail = newNodePhys;
			}
		}
#pragma GCC pop_options

		uintptr_t allocatePhysicalPage(size_t nPages)
		{
			if (!g_nMemoryNodes)
				logger::panic(nullptr, "No more available physical memory left.\n");
			g_pmmLock.Lock();
			uintptr_t flags = saveFlagsAndCLI();
			MemoryNode* node = (MemoryNode*)mapPageTable((uintptr_t*)g_memoryHead);
			MemoryNode* nodePhys = g_memoryHead;
			uintptr_t ret = (uintptr_t)g_memoryHead;
			while (node->nPages < nPages)
			{
				node = node->next;
				if (!node)
					return 0; // Not enough physical memory to satisfy request of nPages.
				nodePhys = node;
				node = (MemoryNode*)mapPageTable((uintptr_t*)node);
			}
			node->nPages -= nPages;
			if (!node->nPages)
			{
				// This node has no free pages after this allocation, so it should be removed.
				MemoryNode *next = node->next, *prev = node->prev;
				if (next)
					((MemoryNode*)mapPageTable((uintptr_t*)next))->prev = prev;
				if (prev)
					((MemoryNode*)mapPageTable((uintptr_t*)prev))->next = next;
				if (g_memoryHead == nodePhys)
					g_memoryHead = next;
				if (g_memoryTail == nodePhys)
					g_memoryTail = prev;
				node->next = nullptr;
				node->prev = nullptr;
			}
			ret = ret + node->nPages * 4096;
			restorePreviousInterruptStatus(flags);
			g_pmmLock.Unlock();
			return ret;
		}
		bool freePhysicalPage(uintptr_t addr, size_t nPages)
		{
			g_pmmLock.Lock();
			MemoryNode* node = (MemoryNode*)mapPageTable((uintptr_t*)addr);
			MemoryNode* nodePhys = (MemoryNode*)addr;
			if (g_memoryTail)
				((MemoryNode*)mapPageTable((uintptr_t*)g_memoryTail))->next = nodePhys;
			if(!g_memoryHead)
				g_memoryHead = nodePhys;
			node->prev = g_memoryTail;
			g_memoryTail = nodePhys;
			node->nPages = nPages;
			g_nMemoryNodes++;
			g_pmmLock.Unlock();
			return true;
		}
	}
}