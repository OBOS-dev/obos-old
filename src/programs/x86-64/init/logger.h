/*
	programs/x86_64/init/logger.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <stdarg.h>

size_t printf(const char* format, ...);
size_t vprintf(const char* format, va_list list);