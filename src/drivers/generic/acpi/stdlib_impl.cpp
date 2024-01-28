/*
	drivers/generic/acpi/stdlib_impl.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <string.h>
#include <memory_manipulation.h>
#include <klog.h>

using namespace obos;

OBOS_EXTERN_C void* memcpy(void* src, const void* dest, size_t size)
{
	return utils::memcpy(src, dest, size);
}
OBOS_EXTERN_C void* memset(void* dest, int _val, size_t size)
{
	_val &= 0xff;
	uint32_t val = _val | (_val << 8) | (_val << 16) | (_val << 24);
	if (!(size % 4))
		return utils::dwMemset((uint32_t*)dest, val, size / 4);
	utils::dwMemset((uint32_t*)dest, val, size / 4);
	byte* destB = (byte*)dest + size/4*4;
	size_t sizeB = size % 4;
	for (size_t i = 0; i < sizeB + 1; i++)
		destB[i] = _val;
	return dest;
}
OBOS_EXTERN_C int memcmp(const void* p1, const void* p2, size_t cnt)
{
	return utils::memcmp(p1, p2, cnt) ? 0 : 1;
}
OBOS_EXTERN_C int strncmp(const char* p1, const char* p2, size_t maxCnt)
{
	if (strnlen(p1, maxCnt) != strnlen(p2, maxCnt))
		return 1;
	size_t sz = strnlen(p1, maxCnt);
	return utils::memcmp(p1, p2, sz);
}
OBOS_EXTERN_C int strcmp(const char* p1, const char* p2)
{
	return utils::strcmp(p1, p2) ? 0 : 1;
}
OBOS_EXTERN_C void* memmove(void* dest, const void* src, size_t size)
{
	if (src == dest)
		return dest;
	if (src > dest)
		return utils::memcpy(dest, src, size);
	byte* _dest = (byte*)dest, * _src = (byte*)src;
	for (size_t i = size; i > 0; i--)
		_dest[i - 1] = _src[i - 1];
	return dest;
	
}
OBOS_EXTERN_C size_t strnlen(const char* str, size_t maxCnt)
{
	size_t i = 0;
	for (; str[i] && i < maxCnt; i++);
	return i;
}
OBOS_EXTERN_C size_t strlen(const char* str)
{
	return utils::strlen(str);
}
static bool isNumber(char ch)
{
	char temp = ch - '0';
	return temp > 0 && temp < 10;
}
static uint64_t dec2bin(const char* str, size_t size)
{
	uint64_t ret = 0;
	for (size_t i = 0; i < size; i++)
	{
		if ((str[i] - '0') < 0 || (str[i] - '0') > 9)
			continue;
		ret *= 10;
		ret += str[i] - '0';
	}
	return ret;
}
static uint64_t hex2bin(const char* str, size_t size)
{
	uint64_t ret = 0;
	str += *str == '\n';
	for (int i = size - 1, j = 0; i > -1; i--, j++)
	{
		char c = str[i];
		uintptr_t digit = 0;
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			digit = c - '0';
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			digit = (c - 'A') + 10;
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
		case 'e':
		case 'f':
			digit = (c - 'a') + 10;
			break;
		default:
			break;
		}
		ret |= digit << (j * 4);
	}
	return ret;
}
static uint64_t oct2bin(const char* str, size_t size)
{
	uint64_t n = 0;
	const char* c = str;
	while (size-- > 0) 
		n = n * 8 + (uint64_t)(*c++ - '0');
	return n;
}
OBOS_EXTERN_C uint64_t strtoull(const char* str, char** endptr, int base)
{
	while (!isNumber(*str++));
	if (!base)
	{
		base = 10;
		if (*(str - 1) == 'x' || *(str - 1) == 'X')
			base = 16;
		else if (*str == '0')
		{
			base = 8;
			str++;
		}
	}
	size_t sz = 0;
	while (isNumber(*str++))
		sz++;
	if (endptr)
		*endptr = (char*)(str + sz);
	switch (base)
	{
	case 10:
		return dec2bin(str, sz);
	case 16:
		return hex2bin(str, sz);
	case 8:
		return oct2bin(str, sz);
	default:
		break;
	}
	return 0xffff'ffff'ffff'ffff;
}
OBOS_EXTERN_C int snprintf(char* dest, size_t maxLen, const char* format, ...)
{
	va_list list;
	va_start(list, format);
	size_t realSize = logger::vsprintf(
		nullptr, format, list
	);
	va_end(list);
	if (!dest || !maxLen)
		return (int)realSize;
	va_start(list, format);
	char* realDest = new char[realSize];
	logger::vsprintf(
		realDest, format, list
	);
	utils::memcpy(dest, realDest, maxLen);
	va_end(list);
	delete[] realDest;
	return realSize;
}