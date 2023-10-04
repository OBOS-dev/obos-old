/*
	oboskrnl/memory_manager/liballoc/liballoc.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#ifdef __cplusplus
extern "C"
{
#endif

PVOID kmalloc(SIZE_T amount);
PVOID kcalloc(SIZE_T nobj, SIZE_T szObj);
PVOID krealloc(PVOID ptr, SIZE_T newSize);
void kfree(PVOID ptr);

#ifdef __cplusplus
}
#endif