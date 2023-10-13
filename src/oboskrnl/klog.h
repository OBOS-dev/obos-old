/*
	oboskrnl/klog.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>
#include <stdarg.h>

#include <console.h>

namespace obos
{
	extern Console g_kernelConsole;
	namespace logger
	{
		enum
		{
			GREY = 0xD3D3D3,
			GREEN = 0x03D12B,
			YELLOW = 0xffcc00,
			ERROR_RED = 0xcc3300,
			PANIC_RED = 0xac1616,
		};

		size_t printf(const char* format, ...);
		size_t vprintf(const char* format, va_list list);
		size_t sprintf(char* dest, const char* format, ...); // TODO: Implement this.

		size_t log(const char* format, ...);
		size_t warning(const char* format, ...);
		size_t error(const char* format, ...);
		[[noreturn]] void panic(const char* format, ...);

		void stackTrace();
	}
}