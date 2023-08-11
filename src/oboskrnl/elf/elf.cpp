/*
	elf.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <elf/elf.h>
#include <elf/elfStructures.h>

#include <memory_manager/paging/allocate.h>

#include <utils/memory.h>

namespace obos
{
	namespace elfLoader
	{
		static DWORD load(PBYTE startAddress, SIZE_T size, UINTPTR_T& baseAddress)
		{
			Elf32_Ehdr* elfHeader = (Elf32_Ehdr*)startAddress;
			Elf32_Phdr* programHeader = (Elf32_Phdr*)(GET_FUNC_ADDR(startAddress) + elfHeader->e_phoff);
			baseAddress = programHeader->p_vaddr;
			for (Elf32_Half i = 0; i < elfHeader->e_phnum; i++, programHeader++)
			{
				if (programHeader->p_type != PT_LOAD)
				{
					// We can't load something that well, can't be loaded.

					continue;
				}
				UINT32_T virtualAllocFlags = 0;
				if ((programHeader->p_flags & PF_W) == PF_W)
					virtualAllocFlags = memory::VirtualAllocFlags::WRITE_ENABLED;
				DWORD nPages = programHeader->p_memsz >> 12;
				if ((programHeader->p_memsz % 4096) != 0)
					nPages++;
				PBYTE dest = (PBYTE)memory::VirtualAlloc((PVOID)programHeader->p_vaddr, nPages, memory::VirtualAllocFlags::WRITE_ENABLED);
				const PBYTE src = startAddress + programHeader->p_offset;
				if (programHeader->p_filesz)
				{
					utils::memcpy(dest + programHeader->p_offset, src, programHeader->p_filesz);
					utils::memzero(dest + programHeader->p_filesz + programHeader->p_offset, (nPages << 12) - programHeader->p_filesz - programHeader->p_offset);
				}
				else
					utils::memzero(dest, nPages << 12);
				memory::MemoryProtect(dest, nPages, virtualAllocFlags);
				if (baseAddress > programHeader->p_vaddr)
					baseAddress = programHeader->p_vaddr;
			}
			return SUCCESS;
		}
		DWORD LoadElfFile(PBYTE startAddress, SIZE_T size, UINTPTR_T& entry, UINTPTR_T& baseAddress)
		{
			if (size <= sizeof(Elf32_Ehdr))
				return INCORRECT_FILE;
			Elf32_Ehdr* elfHeader = (Elf32_Ehdr*)startAddress;
			if (elfHeader->e_ident[EI_MAG0] != ELFMAG0 ||
				elfHeader->e_ident[EI_MAG1] != ELFMAG1 ||
				elfHeader->e_ident[EI_MAG2] != ELFMAG2 ||
				elfHeader->e_ident[EI_MAG3] != ELFMAG3)
				return INCORRECT_FILE;
			if (elfHeader->e_version == EV_NONE)
				return INCORRECT_FILE;
			if(elfHeader->e_ident[EI_CLASS] != ELFCLASS32 || elfHeader->e_ident[EI_DATA] != ELFDATA2LSB)
				return NOT_x86;
			entry = elfHeader->e_entry;
			return load(startAddress, size, baseAddress);
		}
	}
}