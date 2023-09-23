/*
	elf.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

namespace obos
{
	namespace elfLoader
	{
		constexpr DWORD SUCCESS = 0;
		constexpr DWORD INCORRECT_FILE = 1;
		constexpr DWORD NOT_x86 = 2;
		constexpr DWORD NOT_x86_64 = 2;
		constexpr DWORD BASE_ADDRESS_USED = 3;
		DWORD CheckElfFile(PBYTE startAddress, SIZE_T size);
		DWORD LoadElfFile(PBYTE startAddress, SIZE_T size, UINTPTR_T& entry, UINTPTR_T& baseAddress, bool lazyLoad = false);
		//void addr2Line(PVOID addr, STRING& filename, STRING& functionName, SIZE_T& funcOffset, SIZE_T& lineNum);
	}
}