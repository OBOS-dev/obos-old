/*
	programs/x86_64/init/main.cpp
	
	Copyright (c) 2023-2024 Omar Berrow
*/

#include <stdint.h>
#include <stddef.h>

#define OBOS_SYSCALL_IMPL
#include "syscall.h"
#include "liballoc.h"
#include "logger.h"

uintptr_t g_vAllocator = 0xffff'ffff'ffff'ffff;

void *memcpy(void* dest, const void* src, size_t size)
{
	uint8_t* _dest = (uint8_t*)dest;
	const uint8_t* _src = (uint8_t*)src;
	for (size_t i = 0; i < size; i++)
		_dest[i] = _src[i];
	return dest;
}
void* memzero(void* dest, size_t size)
{
	uint8_t* _dest = (uint8_t*)dest;
	for (size_t i = 0; i < size; i++)
		_dest[i] = 0;
	return dest;
}
size_t strlen(const char* str)
{
	size_t ret = 0;
	for (; str[ret]; ret++);
	return ret;
}
size_t strCountToDelimiter(const char* str, char ch, bool stopAtZero)
{
	size_t ret = 0;
	for (; str[ret] != ch && (str[ret] && stopAtZero); ret++);
	return ret;
}
bool memcmp(const void* blk1, const void* blk2, size_t size)
{
	uint8_t* _blk1 = (uint8_t*)blk1;
	uint8_t* _blk2 = (uint8_t*)blk2;
	for (size_t i = 0; i < size; i++)
		if (_blk1[i] != _blk2[i])
			return false;
	return true;
}
bool strcmp(const char* str1, const char* str2)
{
	if (strlen(str1) != strlen(str2))
		return false;
	return memcmp(str1, str2, strlen(str1));
}
static char* itoa(intptr_t value, char* result, int base);
static char* itoa_unsigned(uintptr_t value, char* result, int base);

// Doesn't work with normal file handles.
char* getline(uintptr_t khandle)
{
	char ch[3] = {};
	size_t size = 0;
	char* res = (char*)malloc(++size);
	res[0] = 0;
	while (1)
	{
		while (Eof(khandle));
		ReadFile(khandle, ch, 2, false);
		if (!(ch[1] & (1 << 0)))
		{
			ConsoleOutput(ch);
			if (ch[0] == '\n')
				break;
			if (ch[0] == '\b')
			{
				if (size == 1)
					continue;
				res = (char*)realloc(res, size -= 1);
				res[size - 1] = 0;
				continue;
			}
			res = (char*)realloc(res, ++size);
			res[size - 2] = ch[0];
			res[size - 1] = 0;
		}
		ch[0] = ch[1] = ch[2] = 0;
	}
	return res;
}

void pageFaultHandler()
{
	uintptr_t cr2 = 0;
	asm volatile("mov %%cr2, %0;" : "=r"(cr2) : : );
	printf("\nPage fault! CR2: 0x%p\n", cr2);
	SwapBuffers();
	while (1);
}

int main(char* path)
{
	char* bootRootDirecrtory = (char*)memcpy(path + strlen(path) + 1, path, strCountToDelimiter(path, '/', true) + 1);
	ClearConsole(0);
	RegisterSignal(0, (uintptr_t)pageFaultHandler);
	uintptr_t keyboardHandle = MakeFileHandle();
	OpenFile(keyboardHandle, "K0", 0);
	while (1)
	{
		char* currentLine = getline(keyboardHandle);
		size_t commandSz = strCountToDelimiter(currentLine, ' ', true);
		char* command = (char*)memcpy(new char[commandSz + 1], currentLine, commandSz);
		char* argPtr = currentLine + commandSz + 1;
		if (strcmp(command, "echo"))
		{
			printf("%s\n", argPtr);
		}
		else if (strcmp(command, "cat"))
		{
			uintptr_t filehandle = MakeFileHandle();
			if (!OpenFile(filehandle, argPtr, 1))
			{
				printf("Could not open %s. GetLastError: %d\n", argPtr, GetLastError());
				InvalidateHandle(filehandle);
				continue;
			}
			size_t filesize = GetFilesize(filehandle);
			char* buff = new char[filesize + 1];
			buff[filesize] = 0;
			ReadFile(filehandle, buff, filesize, false);
			ConsoleOutput(buff);
			CloseFileHandle(filehandle);
			InvalidateHandle(filehandle);
			delete[] buff;
		}
		free(currentLine);
		delete[] command;
	}
	return 0;
}

extern "C" void _start(char* path)
{
	g_vAllocator = CreateVirtualAllocator();
	InitializeConsole();
	struct
	{
		alignas(0x10) uint32_t exitCode;
	} par{};
	par.exitCode = main(path);
	if (par.exitCode != 0)
	{
		ConsoleOutput("[Init] main failed with exit code: ");
		char str[12] = {};
		itoa(par.exitCode, str, 10);
		ConsoleOutput(str);
		ConsoleOutput("\n");
	}
	InvalidateHandle(g_vAllocator);
	syscall(12, &par); // ExitThread
}
static void strreverse(char* begin, int size)
{
	int i = 0;
	char tmp = 0;

	for (i = 0; i < size / 2; i++)
	{
		tmp = begin[i];
		begin[i] = begin[size - i - 1];
		begin[size - i - 1] = tmp;
	}
}
// Credit: http://www.strudel.org.uk/itoa/
static char* itoa_unsigned(uintptr_t value, char* result, int base)
{
	// check that the base if valid

	if (base < 2 || base > 16)
	{
		*result = 0;
		return result;
	}

	char* out = result;

	uintptr_t quotient = value;

	do
	{

		uintptr_t abs = /*(quotient % base) < 0 ? (-(quotient % base)) : */(quotient % base);

		*out = "0123456789abcdef"[abs];

		++out;

		quotient /= base;

	} while (quotient);

	strreverse(result, out - result);

	return result;

}
static char* itoa(intptr_t value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, * ptr1 = result, tmp_char;
	intptr_t tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}