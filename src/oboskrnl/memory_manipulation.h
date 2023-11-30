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
		void* memcpy(void* dest, const void* src, size_t size);
		bool memcmp(const void* blk1, const void* blk2, size_t size);
		bool memcmp(const void* blk1, uint32_t val, size_t size);
		// Returns one less than the actual index of the character
		size_t strCountToChar(const char* string, char ch, bool stopAtZero = true);
		size_t strlen(const char* string);
		bool strcmp(const char* str1, const char* str2);
	}
}