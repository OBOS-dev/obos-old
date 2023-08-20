/*
	memory_manager/liballoc/liballoc.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#define PREFIX(fn) k##fn

#ifdef __cplusplus
extern "C" {
#endif

	void* PREFIX(malloc)(SIZE_T size);
	void* PREFIX(calloc)(SIZE_T nObj, SIZE_T szObj);
	void* PREFIX(realloc)(void* blk, SIZE_T newSize);
	SIZE_T PREFIX(getsize)(void* blk);
	void PREFIX(free)(void* blk);

#ifdef __cplusplus
}
#endif

/// <summary>
/// Allocates pages for the allocator.
/// </summary>
/// <param name="location">The location of the pages to be allocated. This is nullptr if the location can be anywhere.</param>
/// <param name="nPages">The amount of pages to be allocated.</param>
/// <returns>The block of memory, or nullptr on failure.</returns>
extern PVOID obos_allocator_allocate_pages(PVOID location, SIZE_T nPages);
/// <summary>
/// Frees pages allocated by the allocator.
/// </summary>
/// <param name="block">The block of memory to free.</param>
/// <param name="nPages">The number of pages to free.</param>
/// <returns>Non-zero on failure.</returns>
extern int obos_allocator_free_pages(PVOID block, SIZE_T nPages);