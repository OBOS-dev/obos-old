/*
	oboskrnl/allocators/liballoc.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>

#include <new>

#include <allocators/liballoc.h>

#include <multitasking/locks/mutex.h>

#include <multitasking/thread.h>

#include <memory_manipulation.h>

#include <allocators/vmm/vmm.h>
#include <allocators/vmm/arch.h>

#if defined(__x86_64__) || defined(_WIN64)
#include <arch/x86_64/memory_manager/virtual/initialize.h>
#endif

#define GET_FUNC_ADDR(addr) reinterpret_cast<uintptr_t>(addr)

#define MIN_PAGES_ALLOCATED 8
#define MEMBLOCK_MAGIC  0x6AB450AA
#define PAGEBLOCK_MAGIC	0x768AADFC
#define MEMBLOCK_DEAD   0x3D793CCD
#define PTR_ALIGNMENT   0x10
#define ROUND_PTR_UP(ptr) (((ptr) + PTR_ALIGNMENT) & ~(PTR_ALIGNMENT - 1))
#define ROUND_PTR_DOWN(ptr) ((ptr) & ~(PTR_ALIGNMENT - 1))

struct memBlock
{
	alignas (0x10) uint32_t magic = MEMBLOCK_MAGIC;
	
	alignas (0x10) size_t size = 0;
	alignas (0x10) void* allocAddr = 0;
	
	alignas (0x10) memBlock* next = nullptr;
	alignas (0x10) memBlock* prev = nullptr;
	
	alignas (0x10) struct pageBlock* pageBlock = nullptr;

	alignas (0x10) void* whoAllocated = nullptr;
};
struct pageBlock
{
	alignas (0x10) uint32_t magic = PAGEBLOCK_MAGIC;
	
	alignas (0x10) pageBlock* next = nullptr;
	alignas (0x10) pageBlock* prev = nullptr;
	
	alignas (0x10) memBlock* firstBlock = nullptr;
	alignas (0x10) memBlock* lastBlock = nullptr;
	alignas (0x10) memBlock* highestBlock = nullptr;
	alignas (0x10) size_t nMemBlocks = 0;
	
	alignas (0x10) size_t nBytesUsed = 0;
	alignas (0x10) size_t nPagesAllocated = 0;
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
		blockAddr = blockAddr + pageBlockTail->nPagesAllocated * g_liballocVirtualAllocator.GetPageSize();
	pageBlock* blk = (pageBlock*)g_liballocVirtualAllocator.VirtualAlloc((void*)blockAddr, nPages * obos::memory::VirtualAllocator::GetPageSize(), 0);
	if(!blk)
		obos::logger::panic(nullptr, "Could not allocate a pageBlock at %p.\n", liballoc_base + totalPagesAllocated * g_liballocVirtualAllocator.GetPageSize());
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
			currentPageBlock = allocateNewPageBlock(amount / g_liballocVirtualAllocator.GetPageSize());
			if (!currentPageBlock)
				return nullptr;
			goto foundPageBlock;
		}

		{
			pageBlock* current = pageBlockHead;

			// We need to look for pageBlock structures.
			while (current)
			{
				//if (((current->nPagesAllocated * g_liballocVirtualAllocator.GetPageSize()) - current->nBytesUsed) >= amountNeeded)
				/*if ((GET_FUNC_ADDR(current->lastBlock->allocAddr) + current->lastBlock->size + sizeof(memBlock) + amountNeeded) <
					(GET_FUNC_ADDR(current) + current->nPagesAllocated * g_liballocVirtualAllocator.GetPageSize()))*/
				if (!obos::memory::_Impl_IsValidAddress(current))
					break;
				if (!obos::memory::_Impl_IsValidAddress(current->lastBlock))
					break;
				// There must be something seriously wrong with this page block if this if statement is executed.
				if (!current->firstBlock || !current->lastBlock)
				{
					current = current->next;
					if (!current)
						break;
					if (!current->firstBlock && !current->lastBlock)
						freePageBlock(current->prev);
					continue;
				}
				// If the code below for seeing if the page block has enough space if modified, don't forget to modify it in krealloc aswell.
				if ((GET_FUNC_ADDR(current->highestBlock->allocAddr) + current->highestBlock->size + amountNeeded) <=
					(GET_FUNC_ADDR(current) + current->nPagesAllocated * g_liballocVirtualAllocator.GetPageSize())
					)
				{
					currentPageBlock = current;
					break;
				}

				current = current->next;
			};
		}

	foundPageBlock:

		// We couldn't find a page block big enough :(
		if (!currentPageBlock)
			currentPageBlock = allocateNewPageBlock(amount / g_liballocVirtualAllocator.GetPageSize());

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
			// This may seem redundant, but it's possible that the loop chose the last block.
			if (block == currentPageBlock->lastBlock)
				currentPageBlock->lastBlock = block->prev;
			if (!block)
			{
				OBOS_ASSERTP(currentPageBlock->highestBlock->magic == MEMBLOCK_MAGIC, "Kernel heap corruption detected for block %p, allocAddr: %p, sizeBlock: %p!", "",
					currentPageBlock->highestBlock,
					currentPageBlock->highestBlock->allocAddr,
					currentPageBlock->highestBlock->size);
				uintptr_t addr = (uintptr_t)currentPageBlock->highestBlock->allocAddr;
				addr += currentPageBlock->highestBlock->size;
				OBOS_ASSERTP(addr > 0xfffffffff0000000, "Kernel heap corruption detected for block %p, allocAddr: %p, sizeBlock: %p!", "",
					currentPageBlock->highestBlock,
					currentPageBlock->highestBlock->allocAddr,
					currentPageBlock->highestBlock->size);
				block = (memBlock*)addr;
			}
		}

		obos::utils::memzero(block, sizeof(*block));
		block->magic = MEMBLOCK_MAGIC;
		block->allocAddr = (void*)((uintptr_t)block + sizeof(memBlock));
		block->size = amount;
		block->pageBlock = currentPageBlock;
		
		if (currentPageBlock->lastBlock)
			currentPageBlock->lastBlock->next = block;
		if(!currentPageBlock->firstBlock)
			currentPageBlock->firstBlock = block;
		block->prev = currentPageBlock->lastBlock;
		if (block > currentPageBlock->highestBlock)
			currentPageBlock->highestBlock = block;
		currentPageBlock->lastBlock = block;
		currentPageBlock->nMemBlocks++;

		currentPageBlock->nBytesUsed += amountNeeded;
		currentPageBlock->lastBlock = block;
#ifdef OBOS_DEBUG
		block->whoAllocated = (void*)__builtin_extract_return_addr(__builtin_return_address(0));
#endif
		return block->allocAddr;
	}
	void* kcalloc(size_t nobj, size_t szObj)
	{
		return obos::utils::memzero(kmalloc(nobj * szObj), nobj * szObj);
	}
	void* krealloc(void* ptr, size_t newSize)
	{
		newSize = ROUND_PTR_UP(newSize);

		if (!ptr)
			return kcalloc(newSize, 1);

		size_t oldSize = 0;

		memBlock* block = (memBlock*)ptr;
		block--;
		if (block->magic != MEMBLOCK_MAGIC)
			return nullptr;
		oldSize = block->size;
		if (oldSize == newSize)
			return ptr; // The block can be kept in the same state.
		if (newSize < oldSize)
		{
			// If the new size is less than the old size.
			// Truncuate the block to the right size.
			block->size = newSize;
			obos::utils::memzero((byte*)ptr + oldSize, oldSize - newSize);
			return ptr;
		}
		if (
			block->pageBlock->highestBlock == block &&
				(GET_FUNC_ADDR(block->pageBlock->highestBlock->allocAddr) + block->pageBlock->highestBlock->size + (newSize - oldSize)) <=
				(GET_FUNC_ADDR(block->pageBlock) + block->pageBlock->nPagesAllocated * g_liballocVirtualAllocator.GetPageSize())
				)
		{
			// If we're the highest block in the page block, and there's still space in the page block, then expand the block size.
			block->size = newSize;
			obos::utils::memzero((byte*)ptr + oldSize, newSize - oldSize);
			return ptr;
		}
		if (((byte*)(block + 1) + oldSize) >= ((byte*)block->pageBlock + block->pageBlock->nPagesAllocated * g_liballocVirtualAllocator.GetPageSize()))
			goto newBlock;
		if (((memBlock*)((byte*)(block + 1) + oldSize))->magic != MEMBLOCK_MAGIC && block->pageBlock->highestBlock != block)
		{
			// This is rather a corrupted block or free space after the block.
			void* blkAfter = ((byte*)(block + 1) + oldSize);
			size_t increment = PTR_ALIGNMENT;
			void* endBlock = nullptr;
			for (endBlock = blkAfter; ((memBlock*)endBlock)->magic != MEMBLOCK_MAGIC; endBlock = (byte*)endBlock + increment);
			size_t nFreeSpace = (byte*)endBlock - (byte*)blkAfter;
			if ((oldSize + nFreeSpace) >= newSize)
			{
				// If we have enough space after the block.
				block->size = newSize;
				obos::utils::memzero((byte*)ptr + oldSize, newSize - oldSize);
				return ptr;
			}
			// Otherwise we'll need a new block.
		}
		newBlock:
		// We need a new block.
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
			if (currentPageBlock->highestBlock == block)
			{
				// Look for the highest block.
				memBlock* highestBlock = nullptr;
				for (auto blk = currentPageBlock->firstBlock; blk; blk = blk->next)
					if (blk > highestBlock)
						highestBlock = blk;
				currentPageBlock->highestBlock = highestBlock;
			}
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

#ifdef OBOS_DEBUG
#define new_impl \
void* ret = kmalloc(count);\
if (!ret)\
	return ret;\
memBlock* blk = (memBlock*)ret;\
blk--;\
blk->whoAllocated = (void*)__builtin_extract_return_addr(__builtin_return_address(0));\
return ret;
#else
#define new_impl \
return kmalloc(count);
#endif

[[nodiscard]] void* operator new(size_t count) noexcept
{
	new_impl
}
[[nodiscard]] void* operator new[](size_t count) noexcept
{
	new_impl
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