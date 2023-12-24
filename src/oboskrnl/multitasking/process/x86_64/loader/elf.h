/*
	multitasking/process/x86_64/loader/elf.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <allocators/vmm/vmm.h>

namespace obos
{
	namespace process
	{
		namespace loader
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
			uint32_t LoadElfFile(byte* startAddress, size_t size, uintptr_t& entry, uintptr_t& baseAddress, memory::VirtualAllocator& allocator, bool lazyLoad = false);
			uint32_t CheckElfFile(byte* startAddress, size_t size, bool setLastError = false);
		}
	}
}
