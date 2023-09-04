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
	SIZE_T sprintf(STRING output, CSTRING format, ...);
	void printf_noFlush(CSTRING format, ...);
	void vprintf(CSTRING format, va_list list);

	void kpanic(PVOID printStackTracePar, PVOID eip, CSTRING format, ...);

	struct stack_frame
	{
		stack_frame* down;
		PVOID eip;
	};

	void printStackTrace(PVOID first, CSTRING prefix = "");
	void disassemble(PVOID eip, CSTRING prefix = "");
	void addr2func(PVOID addr, STRING& str, SIZE_T& functionAddress);
	void addr2file(PVOID addr, STRING& str);
}