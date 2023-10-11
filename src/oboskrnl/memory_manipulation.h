/*
	oboskrnl/memory_manipulation.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <stddef.h>

namespace obos
{
	namespace utils
	{
		uint32_t* dwMemcpy(uint32_t* dest, const uint32_t* src, size_t countDwords);
		uint32_t* dwMemset(uint32_t* dest, uint32_t src, size_t countDwords);
		void* memzero(void* block, size_t size);
		size_t strlen(const char* string);
	}
}