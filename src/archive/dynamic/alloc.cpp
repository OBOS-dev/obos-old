/*
	memory_manager/liballoc/liballoc.h

	Copyright (c) 2023 Omar Berrow
*/

#include <memory_manager/liballoc/liballoc.h>

#include <types.h>
#include <klog.h>
#include <inline-asm.h>

#include <utils/memory.h>

#define OBOS_ALLOCATOR_HEADER_MAGIC		 0x6AB450AA
#define OBOS_ALLOCATOR_DEAD_HEADER_MAGIC 0x768AADFC

// Describes an allocated block of memory.
struct memHeader
{
	struct pageHeader* owner;
	memHeader* next;
	memHeader* prev;
	SIZE_T sizeInMemory;
	SIZE_T sizeAllocated;
	PVOID location;
	DWORD magic;
};
// Describes a group of pages.
struct pageHeader
{
	pageHeader* next;
	pageHeader* prev;
	memHeader* first;
	memHeader* last;
	SIZE_T nPages;
	SIZE_T nMemHeaders;
	SIZE_T nTotalBytesAllocated;
	DWORD magic;
} *basePageHeader = nullptr, * lastPageHeader = nullptr;

#define ALIGNMENT 4
#define ALIGN_MEM(p) ((p >> 2) << 2)

PBYTE baseMemory = nullptr;

extern "C"
{

void* PREFIX(malloc) (SIZE_T size)
{
	if (!size)
		return nullptr;
	obos::EnterKernelSection();
	if (!basePageHeader)
	{
		SIZE_T nPages = size >> 12;

		if (!nPages)
			nPages = 4;

		lastPageHeader = basePageHeader = reinterpret_cast<pageHeader*>(obos_allocator_allocate_pages(nullptr, nPages));
		if (!basePageHeader)
		{
			obos::LeaveKernelSection();
			return nullptr; // Oh no! We're out of memory.
		}
		basePageHeader->next = basePageHeader;
		basePageHeader->prev = nullptr;
		basePageHeader->first = nullptr;
		basePageHeader->last = nullptr;
		basePageHeader->nMemHeaders = 0;
		basePageHeader->nTotalBytesAllocated = 0;
		basePageHeader->nPages = nPages;
		basePageHeader->magic = OBOS_ALLOCATOR_HEADER_MAGIC;
		baseMemory = reinterpret_cast<PBYTE>(basePageHeader);
	}
	memHeader* header = nullptr;
	pageHeader* currentPageHeader = lastPageHeader;
	if (lastPageHeader->nPages << 12 <= size)
	{
		SIZE_T nPages = size >> 12;

		if (size % 4096)
			nPages++;
		if (nPages % 4 && nPages != 1)
			nPages = ((nPages >> 2) + 1) << 2;
		if (nPages == 1)
			nPages = 4;

		pageHeader* nextPageHeader = reinterpret_cast<pageHeader*>(obos_allocator_allocate_pages(reinterpret_cast<PBYTE>(lastPageHeader) + (lastPageHeader->nPages << 12), nPages));
		if (!nextPageHeader)
		{
			obos::LeaveKernelSection();
			return nullptr; // Oh no! We're out of memory.
		}
		nextPageHeader->next = basePageHeader;
		nextPageHeader->prev = lastPageHeader;
		nextPageHeader->first = nullptr;
		nextPageHeader->last = nullptr;
		nextPageHeader->nMemHeaders = 0;
		nextPageHeader->nTotalBytesAllocated = 0;
		nextPageHeader->nPages = nPages;
		nextPageHeader->magic = OBOS_ALLOCATOR_HEADER_MAGIC;
		lastPageHeader->next = nextPageHeader;
		lastPageHeader = nextPageHeader;
	}
lookForHeader:
	header = reinterpret_cast<memHeader*>(lastPageHeader + 1);
	for (; reinterpret_cast<PBYTE>(header) < (reinterpret_cast<PBYTE>(lastPageHeader) + (lastPageHeader->nPages << 12)); header = reinterpret_cast<memHeader*>(
		reinterpret_cast<UINTPTR_T>(header) + header->sizeInMemory))
		if (header->magic != OBOS_ALLOCATOR_HEADER_MAGIC)
			break;
	if (header >= reinterpret_cast<PVOID>(reinterpret_cast<UINTPTR_T>(lastPageHeader) + (lastPageHeader->nPages << 12)))
	{
		SIZE_T nPages = size >> 12;

		if (size % 4096)
			nPages++;
		if (nPages % 4 && nPages != 1)
			nPages = ((nPages >> 2) + 1) << 2;
		if (nPages == 1)
			nPages = 4;

		pageHeader* nextPageHeader = reinterpret_cast<pageHeader*>(obos_allocator_allocate_pages(reinterpret_cast<PBYTE>(lastPageHeader) + (lastPageHeader->nPages << 12), nPages));
		if (!nextPageHeader)
		{
			obos::LeaveKernelSection();
			return nullptr; // Oh no! We're out of memory.
		}
		nextPageHeader->next = basePageHeader;
		nextPageHeader->prev = lastPageHeader;
		nextPageHeader->first = nullptr;
		nextPageHeader->last = nullptr;
		nextPageHeader->nMemHeaders = 0;
		nextPageHeader->nTotalBytesAllocated = 0;
		nextPageHeader->nPages = nPages;
		nextPageHeader->magic = OBOS_ALLOCATOR_HEADER_MAGIC;
		lastPageHeader->next = nextPageHeader;
		lastPageHeader = nextPageHeader;
		currentPageHeader = nextPageHeader;

		lastPageHeader->first = header;
	}
	if (header->magic == OBOS_ALLOCATOR_DEAD_HEADER_MAGIC && header->sizeAllocated < size && basePageHeader->nMemHeaders > 0)
	{
		header = reinterpret_cast<memHeader*>(reinterpret_cast<UINTPTR_T>(header) + header->sizeInMemory);
		goto lookForHeader;
	}
	if (currentPageHeader->magic != OBOS_ALLOCATOR_HEADER_MAGIC)
		obos::kpanic(nullptr, nullptr, kpanic_format("Memory corruption at %p."), currentPageHeader); // Oh no! Someone corrupted the page header.
	if (!lastPageHeader->first)
		lastPageHeader->first = header;
	header->magic = OBOS_ALLOCATOR_HEADER_MAGIC;
	header->sizeAllocated = size;
	header->location = reinterpret_cast<PBYTE>(header) + sizeof(memHeader);
	header->location = reinterpret_cast<PVOID>(ALIGN_MEM(reinterpret_cast<UINTPTR_T>(header->location)));
	header->sizeInMemory = sizeof(memHeader) + size + (reinterpret_cast<UINTPTR_T>(reinterpret_cast<PBYTE>(header) + sizeof(memHeader)) % ALIGNMENT);
	if (header->location >= reinterpret_cast<PVOID>(reinterpret_cast<UINTPTR_T>(lastPageHeader) + (lastPageHeader->nPages << 12)) 
		|| (reinterpret_cast<PBYTE>(header->location) + header->sizeInMemory) >= reinterpret_cast<PBYTE>(reinterpret_cast<UINTPTR_T>(lastPageHeader) + (lastPageHeader->nPages << 12)))
	{
		SIZE_T nPages = size >> 12;

		if (size % 4096)
			nPages++;
		if (nPages % 4 && nPages != 1)
			nPages = ((nPages >> 2) + 1) << 2;
		if (nPages == 1)
			nPages = 4;

		pageHeader* nextPageHeader = reinterpret_cast<pageHeader*>(obos_allocator_allocate_pages(reinterpret_cast<PBYTE>(lastPageHeader) + (lastPageHeader->nPages << 12), nPages));
		if (!nextPageHeader)
		{
			obos::LeaveKernelSection();
			return nullptr; // Oh no! We're out of memory.
		}
		nextPageHeader->next = basePageHeader;
		nextPageHeader->prev = lastPageHeader;
		nextPageHeader->first = nullptr;
		nextPageHeader->last = nullptr;
		nextPageHeader->nMemHeaders = 0;
		nextPageHeader->nTotalBytesAllocated = 0;
		nextPageHeader->nPages = nPages;
		nextPageHeader->magic = OBOS_ALLOCATOR_HEADER_MAGIC;
		lastPageHeader->next = nextPageHeader;
		lastPageHeader = nextPageHeader;
		currentPageHeader = nextPageHeader;
		obos::utils::memcpy(nextPageHeader + 1, header, sizeof(*header));
		obos::utils::memzero(header, sizeof(*header));
		header = reinterpret_cast<memHeader*>(nextPageHeader + 1);

		header->location = reinterpret_cast<PBYTE>(header) + sizeof(memHeader);
		header->location = reinterpret_cast<PVOID>(ALIGN_MEM(reinterpret_cast<UINTPTR_T>(header->location)));

		lastPageHeader->first = header;
	}
	header->next = nullptr;
	header->prev = currentPageHeader->last;
	if (header->prev)
		header->prev->next = header;
	header->owner = currentPageHeader;
	currentPageHeader->last = header;
	currentPageHeader->nMemHeaders++;
	currentPageHeader->nTotalBytesAllocated += header->sizeInMemory;
	obos::LeaveKernelSection();
	return header->location;
}
void* PREFIX(calloc)(SIZE_T nObj, SIZE_T szObj)
{
	if (!szObj || !nObj)
		return nullptr;

	SIZE_T realSize = szObj * nObj;

	return obos::utils::memzero(PREFIX(malloc)(realSize), realSize);
}

static memHeader* lookForHeader(PVOID block)
{
	bool found = false;
	memHeader* currentMemHeader = basePageHeader->first;
	pageHeader* currentPageHeader = basePageHeader;
	for (; currentMemHeader != nullptr; currentMemHeader = currentMemHeader->next)
	{
		if (currentMemHeader->location == block)
		{
			found = true;
			break;
		}
	}
	if (found)
		return currentMemHeader;
	if (basePageHeader->next == basePageHeader)
		return nullptr;
	// Look through the rest...
	for(currentPageHeader = currentPageHeader->next; currentPageHeader != basePageHeader && !found; currentPageHeader = currentPageHeader->next)
	{
		for (; currentMemHeader != nullptr && !found; currentMemHeader = currentMemHeader->next)
		{
			if (currentMemHeader->location == block)
			{
				found = true;
				break;
			}
		}
	}

	return found ? currentMemHeader : nullptr;
}

void* PREFIX(realloc) (void* blk, SIZE_T newSize)
{
	SIZE_T currentSize = PREFIX(getsize)(blk);

	if (!currentSize)
		return nullptr;
	
	obos::EnterKernelSection();

	if (newSize < currentSize)
	{
		memHeader* header = lookForHeader(blk);
		if (!header)
			return nullptr;
		header->sizeAllocated -= (currentSize - newSize);
		header->sizeInMemory -= (currentSize - newSize);
		obos::utils::memzero(reinterpret_cast<PBYTE>(blk) + newSize, currentSize - newSize);
		obos::LeaveKernelSection();
		return blk;
	}

	PVOID newBlock = PREFIX(malloc)(newSize);
	obos::utils::memcpy(newBlock, blk, currentSize);
	PREFIX(free)(blk);
	obos::LeaveKernelSection();
	return newBlock;
}
SIZE_T PREFIX(getsize)(void* blk)
{
	if (!basePageHeader)
		return 0;
	if (!basePageHeader->nMemHeaders)
		return 0;

	memHeader* header = lookForHeader(blk);
	if (!header)
		return 0;
	return header->sizeAllocated;
}
void PREFIX(free) (void* blk)
{
	if (!basePageHeader)
		return;
	if (!basePageHeader->nMemHeaders)
		return;

	obos::EnterKernelSection();

	memHeader* header = lookForHeader(blk);

	if (!header)
	{
		obos::LeaveKernelSection();
		return;
	}

//found:

	header->magic = OBOS_ALLOCATOR_DEAD_HEADER_MAGIC;
	if (header->prev)
		header->prev->next = header->next;
	if (header->next)
		header->next->prev = header->prev;
	header->owner->nTotalBytesAllocated -= header->sizeInMemory;
	header->owner->nMemHeaders--;
	obos::LeaveKernelSection();
}

}
