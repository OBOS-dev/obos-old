/*
	oboskrnl/liballoc/liballoc.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>

#include <x86_64-utils/asm.h>

#include <new>

#include <liballoc/liballoc.h>
#include <arch/x86_64/memory_manager/virtual/allocate.h>

#include <memory_manipulation.h>

#define GET_FUNC_ADDR(addr) reinterpret_cast<uintptr_t>(addr)

#define MIN_PAGES_ALLOCATED 8
#define MEMBLOCK_MAGIC  0x6AB450AA
#define PAGEBLOCK_MAGIC	0x768AADFC
#define PTR_ALIGNMENT 16
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

#if defined(__x86_64__)
static uintptr_t liballoc_base = 0xFFFFFFFFF0000000;
#endif

pageBlock* allocateNewPageBlock(size_t nPages)
{
	nPages += (MIN_PAGES_ALLOCATED - (nPages % MIN_PAGES_ALLOCATED));
	pageBlock* blk = (pageBlock*)obos::memory::VirtualAlloc((void*)(liballoc_base + totalPagesAllocated * 4096), nPages, 0);
	if(!blk)
		obos::logger::panic("Could not allocate a pageBlock at %p.", liballoc_base + totalPagesAllocated * 4096);
	totalPagesAllocated += nPages;
	if (!pageBlockHead)
		pageBlockHead = blk;
	else
		pageBlockHead->next = blk;
	blk->prev = pageBlockTail;
	blk->magic = PAGEBLOCK_MAGIC;
	blk->nPagesAllocated = nPages;
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
	obos::memory::VirtualFree(block, block->nPagesAllocated);
}

// A wrapper for Mutex that unlocks the mutex on destruction.
struct safe_lock
{
	safe_lock() = delete;
	safe_lock(bool* mutex)
	{
		m_mutex = mutex;
	}
	bool Lock()
	{
		if (m_mutex)
		{
			*m_mutex = true;
			return true;
		}
		m_savedFlags = obos::saveFlagsAndCLI();
		return false;
	}
	bool IsLocked()
	{
		if (m_mutex)
			return *m_mutex;
		return m_savedFlags & ((uintptr_t)1<<9);
	}
	void Unlock()
	{
		if (m_mutex && *m_mutex)
			*m_mutex = false;
		if(m_savedFlags)
			obos::restorePreviousInterruptStatus(m_savedFlags);
	}
	~safe_lock()
	{
		Unlock();
	}
private:
	bool* m_mutex = nullptr;
	uintptr_t m_savedFlags = 0;
};

static bool s_allocatorMutex = false;

#define makeSafeLock(vName) safe_lock vName{ &s_allocatorMutex }; vName.Lock();

#ifdef __cplusplus
extern "C" {
#endif

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
			do
			{
				//if (((current->nPagesAllocated * 4096) - current->nBytesUsed) >= amountNeeded)
				if ((GET_FUNC_ADDR(current->lastBlock->allocAddr) + current->lastBlock->size + sizeof(memBlock) + amountNeeded) <
					(GET_FUNC_ADDR(current) + current->nPagesAllocated * 4096))
				{
					currentPageBlock = current;
					break;
				}

				current = current->next;
			} while (current);
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
		{
			block = (memBlock*)(currentPageBlock + 1);
			currentPageBlock->firstBlock = block;
			block->pageBlock = currentPageBlock;
		}
		else
		{
			uintptr_t addr = (uintptr_t)currentPageBlock->lastBlock->allocAddr;
			addr += currentPageBlock->lastBlock->size;
			block = (memBlock*)addr;
		}

		block->magic = MEMBLOCK_MAGIC;
		block->allocAddr = (void*)((uintptr_t)block + sizeof(memBlock));
		block->size = amount;
		block->pageBlock = currentPageBlock;
		block->prev = currentPageBlock->lastBlock;
		if (currentPageBlock->lastBlock)
			currentPageBlock->lastBlock->next = block;
		currentPageBlock->nBytesUsed += amountNeeded;
		currentPageBlock->lastBlock = block;
		currentPageBlock->nMemBlocks++;
			
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
			if(block->next)
				block->next->prev = block->prev;
			if(block->prev)
				block->prev->next = block->next;
			if (currentPageBlock->lastBlock == block)
				currentPageBlock->lastBlock = block->prev;
			obos::utils::memzero(block, totalSize);
		}
		else
			freePageBlock(currentPageBlock);
	}

#ifdef __cplusplus
}
#endif

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