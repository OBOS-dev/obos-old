/*
	init/liballoc.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

void* malloc(size_t amount);
void* calloc(size_t nobj, size_t szObj);
void* realloc(void* ptr, size_t newSize);
void free(void* ptr);

#ifdef __cplusplus
}
#endif
