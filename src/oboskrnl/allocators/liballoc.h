/*
	oboskrnl/allocators/liballoc.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

#ifdef __cplusplus
extern "C"
{
#endif

OBOS_EXPORT void* kmalloc(size_t amount);
OBOS_EXPORT void* kcalloc(size_t nobj, size_t szObj);
OBOS_EXPORT void* krealloc(void* ptr, size_t newSize);
OBOS_EXPORT void kfree(void* ptr);

#ifdef __cplusplus
}
namespace obos
{
	bool CanAllocateMemory();
}
#endif
