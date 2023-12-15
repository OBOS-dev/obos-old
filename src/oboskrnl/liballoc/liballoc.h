/*
	oboskrnl/liballoc/liballoc.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#ifdef __cplusplus
extern "C"
{
#endif

void* kmalloc(size_t amount);
void* kcalloc(size_t nobj, size_t szObj);
void* krealloc(void* ptr, size_t newSize);
void kfree(void* ptr);

#ifdef __cplusplus
}
namespace obos
{
	bool CanAllocateMemory();
}
#endif
