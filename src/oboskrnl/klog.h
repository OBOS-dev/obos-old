/*
	klog.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#include <stdarg.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define kpanic_format(message) "Kernel panic at line " TOSTRING(__LINE__) ", file " __FILE__ ", message:\r\n" message "\r\n"

namespace obos
{
	void printf(CSTRING format, ...);
	void printf_noFlush(CSTRING format, ...);
	void vprintf(CSTRING format, va_list list);

	void kpanic(PVOID printStackTracePar, CSTRING format, ...);

	struct stack_frame
	{
		stack_frame* down;
		PVOID eip;
	};

	void printStackTrace(PVOID first, CSTRING prefix = "");
}