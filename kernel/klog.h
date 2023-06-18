#ifndef __KASSERT_H
#define __KASSERT_H

#include "types.h"

#define KSTR_LITERAL(literal) literal, sizeof(literal) - 1

void setOnKernelPanic(void(*callback)());
void resetOnKernelPanic();

void kpanic(CSTRING message, SIZE_T size);
void kassert(BOOL expression, CSTRING message, SIZE_T size);

void klog_info(CSTRING message);
void klog_warning(CSTRING message);
void klog_error(CSTRING message);

#endif