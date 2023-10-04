/*
	elfStructures.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>


namespace obos
{
	namespace elfLoader
	{
		typedef UINT32_T Elf32_Addr;
		typedef UINT16_T Elf32_Half;
		typedef UINT32_T Elf32_Off;
		typedef  INT32_T Elf32_Sword;
		typedef UINT32_T Elf32_Word;

		constexpr BYTE ELFMAG0 = 0x7f;
		constexpr BYTE ELFMAG1 = 'E';
		constexpr BYTE ELFMAG2 = 'L';
		constexpr BYTE ELFMAG3 = 'F';

		constexpr BYTE ELFCLASSNONE = 0;
		constexpr BYTE ELFCLASS32 = 1;
		constexpr BYTE ELFCLASS64 = 2;

		constexpr BYTE ELFDATANONE = 0;
		constexpr BYTE ELFDATA2LSB = 1;
		constexpr BYTE ELFDATA2MSB = 2;

		constexpr BYTE EV_NONE = 0;
		constexpr BYTE EV_CURRENT = 1;

		constexpr DWORD EI_NIDENT = 16;
		constexpr DWORD EI_MAG0 = 0;
		constexpr DWORD EI_MAG1 = 1;
		constexpr DWORD EI_MAG2 = 2;
		constexpr DWORD EI_MAG3 = 3;
		constexpr DWORD EI_CLASS = 4;
		constexpr DWORD EI_DATA = 5;
		// Must be EV_CURRENT.
		constexpr DWORD EI_VERSION = 6;
		constexpr DWORD EI_PAD = 7;

		struct Elf_CommonEhdr
		{
			unsigned char e_ident[EI_NIDENT];
			Elf32_Half e_type;
			Elf32_Half e_machine;
			Elf32_Word e_version;
		};

		struct Elf32_Ehdr
		{
			Elf_CommonEhdr e_common;
			Elf32_Addr e_entry;
			Elf32_Off  e_phoff;
			Elf32_Off  e_shoff;
			Elf32_Word e_flags;
			Elf32_Half e_ehsize;
			Elf32_Half e_phentsize;
			Elf32_Half e_phnum;
			Elf32_Half e_shentsize;
			Elf32_Half e_shnum;
			Elf32_Half e_shstrndx;
		};

		constexpr UINT32_T PF_R = 0x4;
		constexpr UINT32_T PF_W = 0x2;
		constexpr UINT32_T PF_X = 0x1;

		enum
		{
			PT_NULL,
			PT_LOAD,
			PT_DYNAMIC,
			PT_INTERP,
			PT_NOTE,
			PT_SHLIB,
			PT_PHDR
		};

		struct Elf32_Phdr
		{
			Elf32_Word p_type;
			Elf32_Off  p_offset;
			Elf32_Addr p_vaddr;
			Elf32_Addr p_paddr;
			Elf32_Word p_filesz;
			Elf32_Word p_memsz;
			Elf32_Word p_flags;
			Elf32_Word p_align;
		};

		struct Elf32_Shdr
		{
			Elf32_Word sh_name;
			Elf32_Word sh_type;
			Elf32_Word sh_flags;
			Elf32_Addr sh_addr;
			Elf32_Off  sh_offset;
			Elf32_Word sh_size;
			Elf32_Word sh_link;
			Elf32_Word sh_info;
			Elf32_Word sh_addralign;
			Elf32_Word sh_entsize;
		};

		// 64-bit

		typedef UINTPTR_T Elf64_Addr;
		typedef UINTPTR_T Elf64_Off;
		typedef Elf32_Word Elf64_Word;
		typedef UINT64_T Elf64_Qword;
		typedef Elf32_Half Elf64_Half;

		struct Elf64_Ehdr
		{
			Elf_CommonEhdr e_common;
			Elf64_Addr e_entry;
			Elf64_Off  e_phoff;
			Elf64_Off  e_shoff;
			Elf64_Word e_flags;
			Elf32_Half e_ehsize;
			Elf32_Half e_phentsize;
			Elf32_Half e_phnum;
			Elf32_Half e_shentsize;
			Elf32_Half e_shnum;
			Elf32_Half e_shstrndx;
		};

		struct Elf64_Phdr
		{
			Elf64_Word p_type;
			Elf64_Word p_flags;
			Elf64_Off  p_offset;
			Elf64_Addr p_vaddr;
			Elf64_Addr p_paddr;
			Elf64_Qword p_filesz;
			Elf64_Qword p_memsz;
			Elf64_Qword p_align;
		};

		struct Elf64_Shdr
		{
			Elf64_Word 	sh_name;
			Elf64_Word 	sh_type;
			Elf64_Qword sh_flags;
			Elf64_Addr 	sh_addr;
			Elf64_Off 	sh_offset;
			Elf64_Qword sh_size;
			Elf64_Word 	sh_link;
			Elf64_Word 	sh_info;
			Elf64_Qword sh_addralign;
			Elf64_Qword sh_entsize;
		};
	}
}