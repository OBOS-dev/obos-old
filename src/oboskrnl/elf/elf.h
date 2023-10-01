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
		/// <summary>
		/// Loads the elf file at 'startAddress'
		/// </summary>
		/// <param name="startAddress">The file's data.</param>
		/// <param name="size">The file size.</param>
		/// <param name="entry">A reference to a variable to store the entry point in.</param>
		/// <param name="baseAddress">A reference to a variable to store the base address in.</param>
		/// <param name="lazyLoad">Whether to allocate and copy any of the program header data.</param>
		/// <returns>An error code.</returns>
		DWORD LoadElfFile(PBYTE startAddress, SIZE_T size, UINTPTR_T& entry, UINTPTR_T& baseAddress, bool lazyLoad = false);
	}
}