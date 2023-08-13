/*
	utils/memory.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <utils/memory.h>

#include <memory_manager/paging/allocate.h>

#define TO_PBYTE(addr) ((PBYTE)addr)

namespace obos
{
	namespace utils
	{
		INT memcmp(PCVOID block1, PCVOID block2, SIZE_T size)
		{
			if (block1 == block2)
				return 0;
			{
				bool b1 = !memory::HasVirtualAddress(block1, (size >> 12) + 1);
				bool b2 = !memory::HasVirtualAddress(block2, (size >> 12) + 1);
				if (b1 || b2)
					return -2;
			}
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
	}
}