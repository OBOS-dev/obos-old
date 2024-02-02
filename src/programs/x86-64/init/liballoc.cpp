/*
	programs/x86_64/init/liballoc.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <stdint.h>
#include <stddef.h>

#include "new"
#include "liballoc.h"
#include "syscall.h"
#include "logger.h"

#ifdef __INTELLISENSE__
template<class type>
bool __atomic_compare_exchange_n(type* ptr, type* expected, type desired, bool weak, int success_memorder, int failure_memorder);
#endif

typedef uint8_t byte;

#define GET_FUNC_ADDR(addr) reinterpret_cast<uintptr_t>(addr)
#define ASSERT(expr, msg, ...) if (!(expr)) { printf("Function %s, File %s, Line %d: Assertion failed, \"%s\". " msg "\n", __func__, __FILE__, __LINE__, #expr __VA_ARGS__); while(1); }

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

using AllocFlags = uintptr_t;

extern void* memcpy(void* dest, const void* src, size_t size);
extern void* memzero(void* dest, size_t size);

//#if defined(__x86_64__) || defined(_WIN64)
//static uintptr_t liballoc_base = 0xFFFFFFFFF0000000;
//#endif
extern uintptr_t g_vAllocator;

pageBlock* allocateNewPageBlock(size_t nPages)
{
	nPages += (MIN_PAGES_ALLOCATED - (nPages % MIN_PAGES_ALLOCATED));
	pageBlock* blk = (pageBlock*)VirtualAlloc(g_vAllocator, nullptr, nPages * 4096, 0);
	if (!blk)
		return nullptr;
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
	VirtualFree(g_vAllocator, block, block->nPagesAllocated * 4096);
}

class Mutex
{
public:
	Mutex() = default;
	Mutex(bool) {};

	bool Lock(uint64_t timeout = 0, bool block = true)
	{
		(timeout = timeout);
		if (block && m_locked)
			return false;
		while (m_locked);
		const bool expected = false;
		while (__atomic_compare_exchange_n(&m_locked, (bool*)&expected, true, false, 0, 0));
		return true;

	}
	bool Unlock()
	{
		m_locked = false;
		return true;
	}

	bool Locked() const
	{
		return m_locked;
	}

	bool IsInitialized() const { return m_initialized; }
private:
	bool m_initialized = true;
	bool m_locked = false;
};
// A wrapper for Mutex that unlocks the mutex on destruction.
struct safe_lock
{
	safe_lock() = delete;
	safe_lock(Mutex* mutex)
	{
		m_mutex = mutex;
	}
	bool Lock()
	{
		if (m_mutex)
		{
			m_mutex->Lock();
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
			m_mutex->Unlock();
	}
	~safe_lock()
	{
		Unlock();
	}
private:
	Mutex* m_mutex = nullptr;
};

Mutex g_allocatorMutex;

#define makeSafeLock(vName) if(!g_allocatorMutex.IsInitialized()) { new (&g_allocatorMutex) Mutex{ true }; } safe_lock vName{ &g_allocatorMutex }; vName.Lock();

extern "C" {
	void* malloc(size_t amount)
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
				// If the code below for seeing if the page block has enough space if modified, don't forget to modify it in krealloc as well.
				if ((GET_FUNC_ADDR(current->highestBlock->allocAddr) + current->highestBlock->size + amountNeeded) <=
					(GET_FUNC_ADDR(current) + current->nPagesAllocated * 4096)
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
			// This may seem redundant, but it's possible that the loop chose the last block.
			if (block == currentPageBlock->lastBlock)
				currentPageBlock->lastBlock = block->prev;
			if (!block)
			{
				if (currentPageBlock->highestBlock->magic != MEMBLOCK_MAGIC)
				{
					memBlock* highestBlock = nullptr;
					for (auto blk = currentPageBlock->firstBlock; blk; blk = blk->next)
						if (blk > highestBlock && currentPageBlock->highestBlock != blk)
							highestBlock = blk;
					currentPageBlock->highestBlock = highestBlock;
					ASSERT(currentPageBlock->highestBlock->magic == MEMBLOCK_MAGIC, "Kernel heap corruption detected for block 0x%p, allocAddr: 0x%p, sizeBlock: 0x%p!", "",
						currentPageBlock->highestBlock,
						currentPageBlock->highestBlock->allocAddr,
						currentPageBlock->highestBlock->size);
				}
				uintptr_t addr = (uintptr_t)currentPageBlock->highestBlock->allocAddr;
				addr += currentPageBlock->highestBlock->size;
				// The base of liballoc is now dynamic and not bound to a specific address.
				/*if (addr > 0xfffffffff0000000)
				{
					memBlock* highestBlock = nullptr;
					for (auto blk = currentPageBlock->firstBlock; blk; blk = blk->next)
						if (blk > highestBlock && currentPageBlock->highestBlock != blk)
							highestBlock = blk;
					currentPageBlock->highestBlock = highestBlock;
					ASSERT(addr > 0xfffffffff0000000, "Kernel heap corruption detected for block 0x%p, allocAddr: 0x%p, sizeBlock: 0x%p!", "",
						currentPageBlock->highestBlock,
						currentPageBlock->highestBlock->allocAddr,
						currentPageBlock->highestBlock->size);
				}*/
				block = (memBlock*)addr;
			}
		}

		memzero(block, sizeof(*block));
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
	void* calloc(size_t nobj, size_t szObj)
	{
		return memzero(malloc(nobj * szObj), nobj * szObj);
	}
	void* realloc(void* ptr, size_t newSize)
	{
		if (!newSize)
			return nullptr;

		newSize = ROUND_PTR_UP(newSize);

		if (!ptr)
			return calloc(newSize, 1);

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
			// Truncate the block to the right size.
			block->size = newSize;
			memzero((byte*)ptr + oldSize, oldSize - newSize);
			return ptr;
		}
		if (block->pageBlock->highestBlock == block)
		{
			if (!(((memBlock*)((byte*)(block + 1) + oldSize))->magic != MEMBLOCK_MAGIC))
			{
				memBlock* highestBlock = nullptr;
				for (auto blk = block->pageBlock->firstBlock; blk; blk = blk->next)
					if (blk > highestBlock && block != blk)
						highestBlock = blk;
				block->pageBlock->highestBlock = highestBlock;
				ASSERT(((memBlock*)((byte*)(block + 1) + oldSize))->magic != MEMBLOCK_MAGIC,
					"Kernel heap corruption detected for block 0x%p, allocAddr: 0x%p, sizeBlock: 0x%p!", ,
					block,
					block->allocAddr,
					block->size);
			}
		}
		if (
			block->pageBlock->highestBlock == block &&
				(GET_FUNC_ADDR(block->pageBlock->highestBlock->allocAddr) + block->pageBlock->highestBlock->size + (newSize - oldSize)) <=
				(GET_FUNC_ADDR(block->pageBlock) + block->pageBlock->nPagesAllocated * 4096)
				)
		{
			// If we're the highest block in the page block, and there's still space in the page block, then expand the block size.
			block->size = newSize;
			memzero((byte*)ptr + oldSize, newSize - oldSize);
			return ptr;
		}
		// If the next block crosses page block boundaries, skip checking for that.
		if (((byte*)(block + 1) + oldSize) >= ((byte*)block->pageBlock + block->pageBlock->nPagesAllocated * 4096))
			goto newBlock;
		if (((memBlock*)((byte*)(block + 1) + oldSize))->magic != MEMBLOCK_MAGIC && block->pageBlock->highestBlock != block)
		{
			// This is rather a corrupted block or free space after the block.
			void* blkAfter = ((byte*)(block + 1) + oldSize);
			size_t increment = PTR_ALIGNMENT;
			void* endBlock = nullptr;
			for (endBlock = blkAfter; ((memBlock*)endBlock)->magic != MEMBLOCK_MAGIC &&
				((byte*)endBlock) >= ((byte*)block->pageBlock + block->pageBlock->nPagesAllocated * 4096)
				; endBlock = (byte*)endBlock + increment);
			size_t nFreeSpace = (byte*)endBlock - (byte*)blkAfter;
			if ((oldSize + nFreeSpace) >= newSize)
			{
				// If we have enough space after the block.
				block->size = newSize;
				memzero((byte*)ptr + oldSize, newSize - oldSize);
				return ptr;
			}
			// Otherwise we'll need a new block.
		}
		newBlock:
		// We need a new block.
		void* newBlock = calloc(newSize, 1);
		memcpy(newBlock, ptr, oldSize);
		free(ptr);
		return newBlock;
	}
	void free(void* ptr)
	{
		if (!ptr)
			return;

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
			if (block->next)
				block->next->prev = block->prev;
			if (block->prev)
				block->prev->next = block->next;
			if (currentPageBlock->firstBlock == block)
				block->prev = nullptr;
			if (currentPageBlock->lastBlock == block)
				block->next = nullptr;
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
			memzero(block->allocAddr, block->size);
 			block->magic = MEMBLOCK_DEAD;
		}
		else
			freePageBlock(currentPageBlock);
	}
}

#ifdef OBOS_DEBUG
#define new_impl \
void* ret = malloc(count);\
if (!ret)\
	return ret;\
memBlock* blk = (memBlock*)ret;\
blk--;\
blk->whoAllocated = (void*)__builtin_extract_return_addr(__builtin_return_address(0));\
return ret;
#else
#define new_impl \
return malloc(count);
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
	free(block);
}
void operator delete[](void* block) noexcept
{
	free(block);
}
void operator delete(void* block, size_t)
{
	free(block);
}
void operator delete[](void* block, size_t)
{
	free(block);
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