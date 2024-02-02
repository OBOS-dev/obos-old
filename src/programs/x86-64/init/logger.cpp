/*
	programs/x86_64/init/logger.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

#include "syscall.h"

static char* consoleOutputCallback(const char* buf, void*, int len)
{
	char ch[2] = {};
	for (size_t i = 0; i < (size_t)len; i++)
	{
		ch[0] = buf[i];
		ConsoleOutput(ch);
	}
	return (char*)buf;
}
size_t printf(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	char ch = 0;
	size_t ret = stbsp_vsprintfcb(consoleOutputCallback, nullptr, &ch, format, list);
	va_end(list);
	return ret;
}
size_t vprintf(const char* format, va_list list)
{
	char ch = 0;
	return stbsp_vsprintfcb(consoleOutputCallback, nullptr, &ch, format, list);
}