/*
	oboskrnl/elf/elf.cpp

	Copyright (c) 2023 Omar Berrow
*/

#ifdef __i686__
#include <elf/elf.h>
#include <elf/elfStructures.h>

#include <memory_manager/paging/allocate.h>

#include <utils/memory.h>

#include <boot/multiboot.h>

namespace obos
{
	extern multiboot_info_t* g_multibootInfo;
	namespace elfLoader
	{
		static DWORD load(PBYTE startAddress, SIZE_T, UINTPTR_T& baseAddress)
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
				UINT32_T virtualAllocFlags = memory::VirtualAllocFlags::GLOBAL;
				if ((programHeader->p_flags & PF_W) == PF_W)
					virtualAllocFlags |= memory::VirtualAllocFlags::WRITE_ENABLED;
				DWORD nPages = programHeader->p_memsz >> 12;
				if ((programHeader->p_memsz % 4096) != 0)
					nPages++;
				if (programHeader->p_vaddr >= 0xC0000000 && programHeader->p_vaddr < 0xE0000000)
					return BASE_ADDRESS_USED;
				PBYTE dest = (PBYTE)memory::VirtualAlloc((PVOID)programHeader->p_vaddr, nPages, memory::VirtualAllocFlags::WRITE_ENABLED);
				if (!dest)
					return BASE_ADDRESS_USED;
				const PBYTE src = startAddress + programHeader->p_offset;
				utils::dwMemset((DWORD*)dest, 0, (nPages << 12) >> 2);
				if (programHeader->p_filesz)
				{
					UINTPTR_T offset = programHeader->p_vaddr - (UINTPTR_T)dest;
					dest += offset;
					utils::memcpy(dest, src, programHeader->p_filesz);
				}
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
			if (elfHeader->e_common.e_ident[EI_MAG0] != ELFMAG0 ||
				elfHeader->e_common.e_ident[EI_MAG1] != ELFMAG1 ||
				elfHeader->e_common.e_ident[EI_MAG2] != ELFMAG2 ||
				elfHeader->e_common.e_ident[EI_MAG3] != ELFMAG3)
				return INCORRECT_FILE;
			if (elfHeader->e_common.e_version == EV_NONE)
				return INCORRECT_FILE;
			if(elfHeader->e_common.e_ident[EI_CLASS] != ELFCLASS32 || elfHeader->e_common.e_ident[EI_DATA] != ELFDATA2LSB)
				return NOT_x86;
			entry = elfHeader->e_entry;
			return load(startAddress, size, baseAddress);
		}

		/*static CSTRING getElfString(Elf32_Ehdr* elfHeader, DWORD index)
		{
			CSTRING startAddress = reinterpret_cast<CSTRING>(elfHeader);
			Elf32_Shdr* stringTable = reinterpret_cast<Elf32_Shdr*>(const_cast<STRING>(startAddress) + elfHeader->e_shoff);
			stringTable += elfHeader->e_shstrndx;
			return startAddress + stringTable->sh_offset + index;
		}*/
	}
}
#elif defined(__x86_64__)

#include <elf/elf.h>
#include <elf/elfStructures.h>

#include <types.h>

#include <memory_manager/paging/allocate.h>

#include <utils/memory.h>

namespace obos
{
	namespace elfLoader
	{
		static DWORD load(PBYTE startAddress, SIZE_T, UINTPTR_T& baseAddress, bool lazyLoad)
		{
			baseAddress = 0;
			Elf64_Ehdr* elfHeader = (Elf64_Ehdr*)startAddress;
			Elf64_Phdr* programHeader = (Elf64_Phdr*)(startAddress + elfHeader->e_phoff);
			for (int i = 0; i < elfHeader->e_phnum; i++, programHeader++)
			{
				if (programHeader->p_type != PT_LOAD)
					continue;
				UINTPTR_T allocFlags = memory::VirtualAllocFlags::GLOBAL;
				if (programHeader->p_flags & PF_X)
					allocFlags |= memory::VirtualAllocFlags::EXECUTE_ENABLE;
				if (programHeader->p_flags & PF_W)
					allocFlags |= memory::VirtualAllocFlags::WRITE_ENABLED;
				if (programHeader->p_vaddr > 0xFFFFFFFF80000000 || !programHeader->p_vaddr)
					return BASE_ADDRESS_USED;
				if(!lazyLoad)
				{
					DWORD nPages = programHeader->p_memsz >> 12;
					if ((programHeader->p_memsz % 4096) != 0)
						nPages++;
					PBYTE addr = (PBYTE)memory::VirtualAlloc((PVOID)programHeader->p_vaddr, nPages, memory::VirtualAllocFlags::WRITE_ENABLED | memory::VirtualAllocFlags::GLOBAL);
					if (programHeader->p_filesz)
					{
						UINTPTR_T offset = programHeader->p_vaddr - (UINTPTR_T)addr;
						addr += offset;
						utils::memcpy(addr, startAddress + programHeader->p_offset, programHeader->p_filesz);
					}
					memory::MemoryProtect(addr, nPages, allocFlags);
				}
				if (baseAddress > programHeader->p_vaddr || elfHeader->e_phnum == 1)
					baseAddress = programHeader->p_vaddr;
			}
			return SUCCESS;
		}
		
		DWORD LoadElfFile(PBYTE startAddress, SIZE_T size, UINTPTR_T& entry, UINTPTR_T& baseAddress, bool lazyLoad)
		{
			if (size <= sizeof(Elf64_Ehdr))
				return INCORRECT_FILE;
			Elf64_Ehdr* elfHeader = (Elf64_Ehdr*)startAddress;
			if (elfHeader->e_common.e_ident[EI_MAG0] != ELFMAG0 ||
				elfHeader->e_common.e_ident[EI_MAG1] != ELFMAG1 ||
				elfHeader->e_common.e_ident[EI_MAG2] != ELFMAG2 ||
				elfHeader->e_common.e_ident[EI_MAG3] != ELFMAG3)
				return INCORRECT_FILE;
			if (elfHeader->e_common.e_version != EV_CURRENT)
				return INCORRECT_FILE;
			if (elfHeader->e_common.e_ident[EI_CLASS] != ELFCLASS64 || elfHeader->e_common.e_ident[EI_DATA] != ELFDATA2LSB)
				return NOT_x86_64;
			entry = elfHeader->e_entry;
			return load(startAddress, size, baseAddress, lazyLoad);
		}
	}
}
#endif