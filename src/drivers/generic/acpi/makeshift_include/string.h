#pragma once

#include <int.h>

#ifdef __cplusplus
#define OBOS_EXTERN_C extern "C"
#else
#define OBOS_EXTERN_C
#endif

OBOS_EXTERN_C void* memcpy(void* src, const void* dest, size_t size);
OBOS_EXTERN_C void* memset(void* dest, int val, size_t size);
OBOS_EXTERN_C int memcmp(const void* p1, const void* p2, size_t cnt);
OBOS_EXTERN_C int strncmp(const char* p1, const char* p2, size_t maxCnt);
OBOS_EXTERN_C int strcmp(const char* p1, const char* p2);
OBOS_EXTERN_C void* memmove(void* dest, const void* src, size_t size);
OBOS_EXTERN_C size_t strnlen(const char* str, size_t maxCnt);
OBOS_EXTERN_C size_t strlen(const char* str);
OBOS_EXTERN_C uint64_t strtoull(const char* str, char** endptr, int base);
OBOS_EXTERN_C int snprintf(char* dest, size_t maxLen, const char* format, ...);