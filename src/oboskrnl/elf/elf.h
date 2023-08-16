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
		constexpr DWORD BASE_ADDRESS_USED = 3;
		DWORD LoadElfFile(PBYTE startAddress, SIZE_T size, UINTPTR_T& entry, UINTPTR_T& baseAddress);
	}
}