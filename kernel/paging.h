/*
    paging.h

    Copyright (c) 2023 Omar Berrow
*/

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

void kInitializePaging();
void* kalloc_pages(SIZE_T nPages);
int kfree_pages(PVOID start, SIZE_T nPages);
void reloadPages();

#endif