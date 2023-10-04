/*
	oboskrnl/memory_manager/liballoc/liballoc.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>
#include <inline-asm.h>
#include <klog.h>
#include <error.h>

#include <new>

#include <memory_manager/liballoc/liballoc.h>
#include <memory_manager/paging/allocate.h>

#include <utils/memory.h>

#include <multitasking/mutex/mutex.h>

#define MIN_PAGES_ALLOCATED 8
#define MEMBLOCK_MAGIC  0x6AB450AA
#define PAGEBLOCK_MAGIC	0x768AADFC

struct memBlock
{
	DWORD magic = MEMBLOCK_MAGIC;
	
	SIZE_T size = 0;
	PVOID allocAddr = 0;

	memBlock* next = nullptr;
	memBlock* prev = nullptr;

	PVOID pageBlock = nullptr;
};
struct pageBlock
{
	DWORD magic = PAGEBLOCK_MAGIC;

	pageBlock* next = nullptr;
	pageBlock* prev = nullptr;

	memBlock* firstBlock = nullptr;
	memBlock* lastBlock = nullptr;
	SIZE_T nMemBlocks = 0;

	SIZE_T nBytesUsed = 0;
	SIZE_T nPagesAllocated = 0;
} *pageBlockHead = nullptr, *pageBlockTail = nullptr;

SIZE_T nPageBlocks = 0;
SIZE_T totalPagesAllocated = 0;

using AllocFlags = obos::memory::VirtualAllocFlags;

#if defined(__i686__)
static UINTPTR_T liballoc_base = 0xD0000000;
#elif defined(__x86_64__)
static UINTPTR_T liballoc_base = 0xFFFFFFFFF0000000;
#endif


pageBlock* allocateNewPageBlock(SIZE_T nPages)
{
	nPages += (MIN_PAGES_ALLOCATED - (nPages % MIN_PAGES_ALLOCATED));
	pageBlock* blk = (pageBlock*)obos::memory::VirtualAlloc((PVOID)(liballoc_base + totalPagesAllocated * 4096), nPages, AllocFlags::WRITE_ENABLED);
	if(!blk)
		obos::kpanic(nullptr, nullptr, kpanic_format("Could not allocate a pageBlock at %p."));
	totalPagesAllocated += nPages;
#ifndef __x86_64__
	obos::utils::memzero(blk, nPages * 4096);
#endif
	if (!pageBlockHead)
		pageBlockHead = blk;
	else
		pageBlockHead->next = blk;
	if (pageBlockTail)
		pageBlockTail->prev = blk;
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

extern bool inKernelSection;

static obos::multitasking::Mutex* s_allocatorMutex;

// A wrapper for Mutex that unlocks the mutex on destruction.
struct safe_lock
{
	safe_lock() = delete;
	safe_lock(obos::multitasking::Mutex* mutex)
	{
		m_mutex = mutex;
	}
	bool Lock(bool waitIfLocked = true)
	{
		if (m_mutex)
		{
			m_mutex->Lock(waitIfLocked);
			return m_mutex->IsLocked();
		}
		return false;
	}
	bool IsLocked()
	{
		if (m_mutex)
			return m_mutex->IsLocked();
		return false;
	}
	void Unlock()
	{
		if (m_mutex && m_mutex->IsLocked())
			m_mutex->Unlock();
	}
	~safe_lock()
	{
		Unlock();
	}
private:
	obos::multitasking::Mutex* m_mutex = nullptr;
};

#define makeSafeLock(vName) safe_lock vName{ s_allocatorMutex }; if(!inKernelSection) vName .Lock();
static obos::multitasking::Mutex mutex;

void allocate_mutex()
{
	if (!s_allocatorMutex)
	{
		s_allocatorMutex = &mutex;
		new(s_allocatorMutex) obos::multitasking::Mutex{};
	}
}

#ifdef __cplusplus
extern "C" {
#endif

	PVOID kmalloc(SIZE_T amount)
	{
		allocate_mutex();
		makeSafeLock(lock);
		
		amount += (sizeof(UINTPTR_T) - (amount % sizeof(UINTPTR_T)));

		pageBlock* currentPageBlock = nullptr;

		// Choose a pageBlock that fits a block with "amount"
		
		const SIZE_T amountNeeded = amount + sizeof(memBlock);

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
				if ((current->nBytesUsed - current->nPagesAllocated * 4096) >= amountNeeded)
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
			UINTPTR_T addr = (UINTPTR_T)currentPageBlock->lastBlock->allocAddr;
			addr += currentPageBlock->lastBlock->size;
			block = (memBlock*)addr;
		}

		block->magic = MEMBLOCK_MAGIC;
		block->allocAddr = (PVOID)((UINTPTR_T)block + sizeof(memBlock));
		block->pageBlock = currentPageBlock;
		block->prev = currentPageBlock->lastBlock;
		if (currentPageBlock->lastBlock)
			currentPageBlock->lastBlock->next = block;
		block->size = amount;
		currentPageBlock->nBytesUsed += amountNeeded;
		currentPageBlock->lastBlock = block;
		currentPageBlock->nMemBlocks++;
			
		return block->allocAddr;
	}
	PVOID kcalloc(SIZE_T nobj, SIZE_T szObj)
	{
		allocate_mutex();
		return obos::utils::memzero(kmalloc(nobj * szObj), nobj * szObj);
	}
	PVOID krealloc(PVOID ptr, SIZE_T newSize)
	{
		allocate_mutex();
		if (!ptr)
		{
			obos::SetLastError(obos::OBOS_ERROR_INVALID_PARAMETER);
			return nullptr;
		}

		SIZE_T oldSize = 0;
		
		memBlock* block = (memBlock*)ptr;
		block--;
		if (block->magic != MEMBLOCK_MAGIC)
		{
			obos::SetLastError(obos::OBOS_ERROR_INVALID_PARAMETER);
			return nullptr;
		}
		oldSize = block->size;

		PVOID newBlock = kcalloc(newSize, 1);
		obos::utils::memcpy(newBlock, ptr, oldSize);
		kfree(ptr);
		return newBlock;
	}
	void kfree(PVOID ptr)
	{
		allocate_mutex();
		makeSafeLock(lock);

		memBlock* block = (memBlock*)ptr;
		block--;
		if (block->magic != MEMBLOCK_MAGIC)
		{
			obos::SetLastError(obos::OBOS_ERROR_INVALID_PARAMETER);
			return;
		}

		pageBlock* currentPageBlock = (pageBlock*)block->pageBlock;

		const SIZE_T totalSize = sizeof(memBlock) + block->size;

		currentPageBlock->nBytesUsed -= totalSize;

		if (currentPageBlock->nMemBlocks--)
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