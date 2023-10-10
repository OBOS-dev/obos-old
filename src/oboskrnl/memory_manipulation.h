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
		void* memzero(void* block, size_t size);
		size_t strlen(const char* string);
	}
}