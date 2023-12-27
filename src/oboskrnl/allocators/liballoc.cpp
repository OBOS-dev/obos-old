/*
	oboskrnl/allocators/liballoc.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>

#include <new>

#include <allocators/liballoc.h>

#include <multitasking/locks/mutex.h>

#include <multitasking/thread.h>

#include <memory_manipulation.h>

#include <allocators/vmm/vmm.h>

#if defined(__x86_64__) || defined(_WIN64)
#include <arch/x86_64/memory_manager/virtual/initialize.h>
#endif

#define GET_FUNC_ADDR(addr) reinterpret_cast<uintptr_t>(addr)

#define MIN_PAGES_ALLOCATED 8
#define MEMBLOCK_MAGIC  0x6AB450AA
#define PAGEBLOCK_MAGIC	0x768AADFC
#define MEMBLOCK_DEAD   0x3D793CCD
#define PTR_ALIGNMENT   16
#define ROUND_PTR_UP(ptr) (((ptr) + PTR_ALIGNMENT) & ~(PTR_ALIGNMENT - 1))
#define ROUND_PTR_DOWN(ptr) ((ptr) & ~(PTR_ALIGNMENT - 1))

struct memBlock
{
	uint32_t magic = MEMBLOCK_MAGIC;
	
	size_t size = 0;
	void* allocAddr = 0;

	memBlock* next = nullptr;
	memBlock* prev = nullptr;

	void* pageBlock = nullptr;
};
struct pageBlock
{
	uint32_t magic = PAGEBLOCK_MAGIC;

	pageBlock* next = nullptr;
	pageBlock* prev = nullptr;

	memBlock* firstBlock = nullptr;
	memBlock* lastBlock = nullptr;
	size_t nMemBlocks = 0;

	size_t nBytesUsed = 0;
	size_t nPagesAllocated = 0;
} *pageBlockHead = nullptr, *pageBlockTail = nullptr;

size_t nPageBlocks = 0;
size_t totalPagesAllocated = 0;

using AllocFlags = obos::memory::PageProtectionFlags;

#if defined(__x86_64__) || defined(_WIN64)
static uintptr_t liballoc_base = 0xFFFFFFFFF0000000;
#endif
obos::memory::VirtualAllocator g_liballocVirtualAllocator{ nullptr };

pageBlock* allocateNewPageBlock(size_t nPages)
{
	nPages += (MIN_PAGES_ALLOCATED - (nPages % MIN_PAGES_ALLOCATED));
	uintptr_t blockAddr = (uintptr_t)pageBlockTail;
	if (!blockAddr)
		blockAddr = liballoc_base;
	else
		blockAddr = blockAddr + pageBlockTail->nPagesAllocated * 4096;
	pageBlock* blk = (pageBlock*)g_liballocVirtualAllocator.VirtualAlloc((void*)blockAddr, nPages * obos::memory::VirtualAllocator::GetPageSize(), 0);
	if(!blk)
		obos::logger::panic(nullptr, "Could not allocate a pageBlock at %p.", liballoc_base + totalPagesAllocated * 4096);
	blk->magic = PAGEBLOCK_MAGIC;
	blk->nPagesAllocated = nPages;
	totalPagesAllocated += nPages;
	if (pageBlockTail)
		pageBlockTail->next = blk;
	if(!pageBlockHead)
		pageBlockHead = blk;
	blk->prev = pageBlockTail;
	pageBlockTail = blk;
	nPageBlocks++;
	return blk;
}
void freePageBlock(pageBlock* block)
{
	if (block->next)
		block->next->prev = block->prev;
	if (block->prev)
		block->prev->next = block->next;
	if (pageBlockTail == block)
		pageBlockTail = block->prev;
	nPageBlocks--;
	totalPagesAllocated -= block->nPagesAllocated;
	g_liballocVirtualAllocator.VirtualFree(block, block->nPagesAllocated * obos::memory::VirtualAllocator::GetPageSize());
}

// A wrapper for Mutex that unlocks the mutex on destruction.
struct safe_lock
{
	safe_lock() = delete;
	safe_lock(obos::locks::Mutex* mutex)
	{
		m_mutex = mutex;
	}
	bool Lock()
	{
		if (m_mutex)
		{
			m_mutex->Lock();
			previousInterruptStatus = obos::thread::stopTimer();
			return true;
		}
		return false;
	}
	bool IsLocked()
	{
		if (m_mutex)
			return m_mutex->Locked();
		return false;
	}
	void Unlock()
	{
		if (m_mutex)
		{
			m_mutex->Unlock();
			obos::thread::startTimer(previousInterruptStatus);
			previousInterruptStatus = 0;
		}
	}
	~safe_lock()
	{
		Unlock();
	}
private:
	obos::locks::Mutex* m_mutex = nullptr;
	uint64_t previousInterruptStatus;
};

obos::locks::Mutex g_allocatorMutex;

#define makeSafeLock(vName) if(!g_allocatorMutex.IsInitialized()) { new (&g_allocatorMutex) obos::locks::Mutex{ true }; } safe_lock vName{ &g_allocatorMutex }; vName.Lock();

extern "C" {
	void* kmalloc(size_t amount)
	{
		makeSafeLock(lock);

		if (!amount)
			return nullptr;

		amount = ROUND_PTR_UP(amount);

		pageBlock* currentPageBlock = nullptr;

		// Choose a pageBlock that fits a block with "amount"

		size_t amountNeeded = amount + sizeof(memBlock);

		if (nPageBlocks == 0)
		{
			currentPageBlock = allocateNewPageBlock(amount / 4096);
			if (!currentPageBlock)
				return nullptr;
			goto foundPageBlock;
		}

		{
			pageBlock* current = pageBlockHead;

			// We need to look for pageBlock structures.
			while (current)
			{
				//if (((current->nPagesAllocated * 4096) - current->nBytesUsed) >= amountNeeded)
				/*if ((GET_FUNC_ADDR(current->lastBlock->allocAddr) + current->lastBlock->size + sizeof(memBlock) + amountNeeded) <
					(GET_FUNC_ADDR(current) + current->nPagesAllocated * 4096))*/
				if ((GET_FUNC_ADDR(current->lastBlock->allocAddr) + current->lastBlock->size + amountNeeded) <=
					(GET_FUNC_ADDR(current) + current->nPagesAllocated * 4096)
					)
				{
					currentPageBlock = current;
					break;
				}
#ifdef OBOS_DEBUG
				else
					asm("nop");
#endif

				current = current->next;
			};
		}

	foundPageBlock:

		// We couldn't find a page block big enough :(
		if (!currentPageBlock)
			currentPageBlock = allocateNewPageBlock(amount / 4096);

		if (!currentPageBlock)
			return nullptr;
		memBlock* block = nullptr;

		// If this is an empty block.
		if (!currentPageBlock->firstBlock)
			block = (memBlock*)(currentPageBlock + 1);
		else
		{
			// Look for a free address.
			memBlock* current = (memBlock*)(currentPageBlock + 1);
			while (current != nullptr && current < currentPageBlock->lastBlock)
			{
				if (current->magic != MEMBLOCK_DEAD)
				{
					current = current->next;
					continue;
				}
				if (current->size >= (amountNeeded - sizeof(memBlock)))
				{
					block = current;
					break;
				}
				current = current->next;
			}
			if (currentPageBlock->lastBlock->magic == MEMBLOCK_DEAD && !block)
			{
				block = currentPageBlock->lastBlock;
			}
			// This may seem recursive, but it's possible that the loop chose the last block.
			if (block == currentPageBlock->lastBlock)
				currentPageBlock->lastBlock = block->prev;
			if (!block)
			{
				OBOS_ASSERTP(currentPageBlock->lastBlock->magic == MEMBLOCK_MAGIC, "Kernel heap corruption detected for block %p, allocAddr: %p, sizeBlock: %p!", "",
					currentPageBlock->lastBlock,
					currentPageBlock->lastBlock->allocAddr,
					currentPageBlock->lastBlock->size);
				uintptr_t addr = (uintptr_t)currentPageBlock->lastBlock->allocAddr;
				addr += currentPageBlock->lastBlock->size;
				OBOS_ASSERTP(addr > 0xfffffffff0000000, "Kernel heap corruption detected for block %p, allocAddr: %p, sizeBlock: %p!", "",
					currentPageBlock->lastBlock,
					currentPageBlock->lastBlock->allocAddr,
					currentPageBlock->lastBlock->size);
				block = (memBlock*)addr;
			}
		}

		block->magic = MEMBLOCK_MAGIC;
		block->allocAddr = (void*)((uintptr_t)block + sizeof(memBlock));
		block->size = amount;
		block->pageBlock = currentPageBlock;
		
		if (currentPageBlock->lastBlock)
			currentPageBlock->lastBlock->next = block;
		if(!currentPageBlock->firstBlock)
			currentPageBlock->firstBlock = block;
		block->prev = currentPageBlock->lastBlock;
		currentPageBlock->lastBlock = block;
		currentPageBlock->nMemBlocks++;

		currentPageBlock->nBytesUsed += amountNeeded;
		currentPageBlock->lastBlock = block;
		return block->allocAddr;
	}
	void* kcalloc(size_t nobj, size_t szObj)
	{
		return obos::utils::memzero(kmalloc(nobj * szObj), nobj * szObj);
	}
	void* krealloc(void* ptr, size_t newSize)
	{
		if (!ptr)
			return kcalloc(newSize, 1);

		size_t oldSize = 0;

		memBlock* block = (memBlock*)ptr;
		block--;
		if (block->magic != MEMBLOCK_MAGIC)
			return nullptr;
		oldSize = block->size;

		void* newBlock = kcalloc(newSize, 1);
		obos::utils::memcpy(newBlock, ptr, oldSize);
		kfree(ptr);
		return newBlock;
	}
	void kfree(void* ptr)
	{
		makeSafeLock(lock);

		memBlock* block = (memBlock*)ptr;
		block--;
		if (block->magic != MEMBLOCK_MAGIC)
			return;

		pageBlock* currentPageBlock = (pageBlock*)block->pageBlock;

		const size_t totalSize = sizeof(memBlock) + block->size;

		currentPageBlock->nBytesUsed -= totalSize;

		if (--currentPageBlock->nMemBlocks)
		{
			if (currentPageBlock->lastBlock == block)
			{
				block->next = nullptr;
				goto next1;
			}
			if (block->next)
			{
				OBOS_ASSERTP(block->next > (void*)0xfffffffff0000000, "Kernel heap corruption detected for block %p, allocAddr: %p, sizeBlock: 0x%X!", "", block, block->allocAddr, block->size);
				block->next->prev = block->prev;
			}
		next1:
			if (currentPageBlock->firstBlock == block)
			{
				block->prev = nullptr;
				goto next2;
			}
			if (block->prev)
			{
				OBOS_ASSERTP(block->prev > (void*)0xfffffffff0000000, "Kernel heap corruption detected for block %p, allocAddr: %p, sizeBlock: 0x%X!", "", block, block->allocAddr, block->size);
				block->prev->next = block->next;
			}
		next2:
			if (currentPageBlock->lastBlock == block)
				currentPageBlock->lastBlock = block->prev;
			if (currentPageBlock->firstBlock == block)
				currentPageBlock->firstBlock = block->next;
			obos::utils::memzero(block->allocAddr, block->size);
			block->magic = MEMBLOCK_DEAD;
		}
		else
			freePageBlock(currentPageBlock);
	}
}

namespace obos
{
	bool CanAllocateMemory()
	{
#if defined(__x86_64__) || defined(_WIN64)
		return obos::memory::g_initialized;
#endif
	}
}

[[nodiscard]] void* operator new(size_t count) noexcept
{
#ifndef OBOS_RELEASE
	return obos::utils::memzero(kmalloc(count), count);
#else
	return kmalloc(count);
#endif
}
[[nodiscard]] void* operator new[](size_t count) noexcept
{
	return operator new(count);
}
void operator delete(void* block) noexcept
{
	kfree(block);
}
void operator delete[](void* block) noexcept
{
	kfree(block);
}
void operator delete(void* block, size_t)
{
	kfree(block);
}
void operator delete[](void* block, size_t)
{
	kfree(block);
}

[[nodiscard]] void* operator new(size_t, void* ptr) noexcept
{
	return ptr;
}
[[nodiscard]] void* operator new[](size_t, void* ptr) noexcept
{
	return ptr;
}
void operator delete(void*, void*) noexcept
{}
void operator delete[](void*, void*) noexcept
{}