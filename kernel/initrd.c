/*
	initrd.c

	Copyright (c) 2023 Omar Berrow
*/

#include "initrd.h"
#include "multitasking.h"
#include "error.h"
#include "types.h"

#include "list/list.h"

//static PVOID memset(PVOID block, CHAR ch, SIZE_T size)
//{
//	PCHAR _block = block;
//	for (int i = 0; i < size; _block[i++] = ch);
//	return block;
//}
static BOOL memcmp(PVOID block1, PVOID block2, SIZE_T size)
{
	PCHAR _block1 = block1;
	PCHAR _block2 = block2;
	for (int i = 0; i < size; i++)
		if (_block1[i] != _block2[i])
			return FALSE;
	return TRUE;
}
static SIZE_T strlen(CSTRING str, SIZE_T maxLen)
{
	SIZE_T i = 0;
	if (!maxLen)
		maxLen = 0xFFFFFFFF;
	for (; str[i] && i < maxLen; i++);
	return i;
}
//static BOOL strcmp(CSTRING str1, CSTRING str2)
//{
//	// If the pointers are the same.
//	if (str1 == str2)
//		return TRUE;
//	SIZE_T str1Len = strlen(str1, 0);
//	SIZE_T str2Len = strlen(str2, 0);
//	if (str1Len != str2Len)
//		return FALSE;
//	return memcmp(str1, str2, str1Len);
//}
//static BOOL memcontains(PVOID block, CHAR ch, SIZE_T size)
//{
//	PCHAR blk = block;
//	for (int i = 0; i < size; i++)
//		if (blk[i] == ch)
//			return TRUE;
//	return FALSE;
//}

static UINT8_T* s_initRDPointer;

/// <summary>
/// Converts an octal string to an integer.
/// </summary>
/// <param name="str">The string.</param>
/// <param name="size">The string's size.</param>
/// <returns></returns>
static int oct2bin(unsigned char* str, int size)
{
	int n = 0;
	unsigned char* c = str;
	while (size-- > 0)
	{
		n *= 8;
		n += *c - '0';
		c++;
	}
	return n;
}

// Reads from the USTAR ramdisk.
static int __Impl_Readfile(const char* filename, char** out) {
	unsigned char* ptr = s_initRDPointer;

	while (memcmp(ptr + 257, "ustar", 5)) 
	{
		int filesize = oct2bin(ptr + 0x7c, 11);
		if (memcmp((PVOID)ptr, (PVOID)filename, strlen(filename, 100) + 1)) 
		{
			*out = (char*)(ptr + 512);
			return filesize;
		}
		ptr += (((filesize + 511) / 512) + 1) * 512;
	}
	return -1;
}


VOID InitializeInitRD(PVOID startAddress)
{
	s_initRDPointer = startAddress;
}
BOOL ReadInitRDFile(CSTRING filePath, SIZE_T bytesToRead, SIZE_T* bytesRead, STRING output, SIZE_T offset)
{
	if (filePath[0] == '/')
		filePath++;
	char* buf = NULLPTR;
	int ret = __Impl_Readfile(filePath, &buf);
	if(ret == -1)
	{
		if (!s_initRDPointer)
		{
			SetLastError(OBOS_ERROR_INVALID_HANDLE);
			return FALSE;
		}
		else
		{
			SetLastError(OBOS_ERROR_FILE_NOT_FOUND);
			return FALSE;
		}
	}
	if (output)
	{
		SIZE_T i;
		for (i = offset; i < bytesToRead && i < ret; i++)
			output[i - offset] = buf[i];
		*bytesRead = i;
	}
	return TRUE;
}
