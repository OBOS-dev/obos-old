#ifndef __OBOS_KALLOC_H
#define __OBOS_KALLOC_H
#include "types.h"

typedef struct __memory_block
{
    INT size;
    PCVOID start;
    PCVOID end;
    BOOL isInUse;
} memory_block;
// Initializes the memory table.
void kmeminit();
// Gets a block for use by the kernel
void* kfindmemblock(SIZE_T size, SIZE_T* real_size);
// Initalizes the heap.
void kheapinit(PVOID block, SIZE_T blockSize);
// Allocates a block of memory.
// expectedResize is the size to expect the block to be reallocated to. This is used the next time a block is allocated.
// Returns 0xFFFFFFFF on failure.
PVOID kheapalloc(SIZE_T size, SIZE_T expectedResize);
#endif