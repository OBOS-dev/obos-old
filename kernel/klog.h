#ifndef __KASSERT_H
#define __KASSERT_H

#include "types.h"

#include <stdarg.h>

#define KSTR_LITERAL(literal) literal, sizeof(literal) - 1

void setOnKernelPanic(void(*callback)());
void resetOnKernelPanic();

// "output" should be 64 bytes + a nul character.
//void lookupFunctionName(void* address, char* output);
void printStackTrace();

void kpanic(CSTRING message, SIZE_T size);
void kassert(BOOL expression, CSTRING message, SIZE_T size);

void printf(CSTRING format, ...) attribute(format(printf, 1, 2));
void vprintf(CSTRING format, va_list list);
// For the format string, do not use it like printf. Look in the code to see how to make the format string.
void klog_info(CSTRING format, ...) attribute(format(printf, 1, 2));
// For the format string, do not use it like printf. Look in the code to see how to make the format string.
void klog_warning(CSTRING format, ...) attribute(format(printf, 1, 2));
// For the format string, do not use it like printf. Look in the code to see how to make the format string.
void klog_error(CSTRING format, ...) attribute(format(printf, 1, 2));

#endif