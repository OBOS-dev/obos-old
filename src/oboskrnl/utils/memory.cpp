/*
	utils/memory.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <utils/memory.h>
#include <utils/smart_ptr.h>

#include <memory_manager/paging/allocate.h>

#define TO_PBYTE(addr) ((PBYTE)addr)

namespace obos
{
	namespace utils
	{
		PVOID memset(PVOID block, BYTE ch, SIZE_T size)
		{
			if (!memory::HasVirtualAddress(block, (size >> 12) + 1))
				return nullptr;
			for (SIZE_T i = 0; i < size; TO_PBYTE(block)[i++] = ch);
			return block;
		}
		PVOID memcpy(PVOID dest, PCVOID src, SIZE_T size)
		{
			if (!memory::HasVirtualAddress(dest, (size >> 12) + 1) || !memory::HasVirtualAddress(src, (size >> 12) + 1))
				return nullptr;
			for (SIZE_T i = 0; i < size; i++)
				TO_PBYTE(dest)[i] = TO_PBYTE(src)[i];
			return dest;
		}
		INT memcmp(PCVOID block1, PCVOID block2, SIZE_T size)
		{
			if (block1 == block2)
				return 0;
			if (!memory::HasVirtualAddress(block1, (size >> 12) + 1) || !memory::HasVirtualAddress(block2, (size >> 12) + 1))
				return -2;
			for (SIZE_T i = 0; i < size; i++)
				if (TO_PBYTE(block1)[i] < TO_PBYTE(block2)[i])
					return -1;
				else if (TO_PBYTE(block1)[i] > TO_PBYTE(block2)[i])
					return -1;
			return 0;
		}
		PVOID memchr(PVOID block, BYTE ch, SIZE_T size)
		{
			if (!memory::HasVirtualAddress(block, (size >> 12) + 1))
				return nullptr;
			for (SIZE_T i = 0; i < size; i++)
				if (TO_PBYTE(block)[i] == ch)
					return TO_PBYTE(block) + i;
			return block;
		}
		PCVOID memchr(PCVOID block, BYTE ch, SIZE_T size)
		{
			return (PCVOID)memchr((PVOID)block, ch, size);
		}

		SIZE_T strlen(CSTRING str)
		{
			if (!memory::HasVirtualAddress(str, 1))
				return (SIZE_T)-1;
			SIZE_T i = 0;
			for (; str[i]; i++);
			return i;
		}
	}
}