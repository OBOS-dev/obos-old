/*
	utils/memory.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#define GET_FUNC_ADDR(f) ((UINTPTR_T)(f))
#define ZeroMemory(block) memset(block, 0, sizeof(block))

namespace obos
{
	namespace utils
	{
		PVOID memset(PVOID block, BYTE ch, SIZE_T size);
		PVOID memcpy(PVOID dest, PCVOID src, SIZE_T size);
		INT memcmp(PCVOID block1, PCVOID block2, SIZE_T size);
		PVOID memchr(PVOID block, BYTE ch, SIZE_T size);
		PCVOID memchr(PCVOID block, BYTE ch, SIZE_T size);

		SIZE_T strlen(CSTRING str);
	}
}