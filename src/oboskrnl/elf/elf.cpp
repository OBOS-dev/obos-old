/*
	elf.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <elf/elf.h>
#include <elf/elfStructures.h>

#include <memory_manager/paging/allocate.h>

#include <utils/memory.h>

#include <boot/multiboot.h>

#include <klog.h>
#include <inline-asm.h>

namespace obos
{
	extern multiboot_info_t* g_multibootInfo;
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

		/*static CSTRING getElfString(Elf32_Ehdr* elfHeader, DWORD index)
		{
			CSTRING startAddress = reinterpret_cast<CSTRING>(elfHeader);
			Elf32_Shdr* stringTable = reinterpret_cast<Elf32_Shdr*>(const_cast<STRING>(startAddress) + elfHeader->e_shoff);
			stringTable += elfHeader->e_shstrndx;
			return startAddress + stringTable->sh_offset + index;
		}*/

		/*typedef struct __attribute__((packed)) {
			UINT32_T length;
			UINT16_T version;
			UINT32_T header_length;
			UINT8_T min_instruction_length;
			UINT8_T default_is_stmt;
			INT8_T line_base;
			UINT8_T line_range;
			UINT8_T opcode_base;
			UINT8_T std_opcode_lengths[12];
		} DebugLineHeader;

		void addr2Line(PVOID addr, STRING& filename, STRING& functionName, SIZE_T& funcOffset, SIZE_T& lineNum)
		{
			PBYTE startAddress = reinterpret_cast<PBYTE>(reinterpret_cast<multiboot_module_t*>(g_multibootInfo->mods_addr)[4].mod_start);
			//PBYTE endAddress = reinterpret_cast<PBYTE>(reinterpret_cast<multiboot_module_t*>(g_multibootInfo->mods_addr)[4].mod_end);
			Elf32_Ehdr* elfHeader = (Elf32_Ehdr*)startAddress;
			if (elfHeader->e_ident[EI_MAG0] != ELFMAG0 ||
				elfHeader->e_ident[EI_MAG1] != ELFMAG1 ||
				elfHeader->e_ident[EI_MAG2] != ELFMAG2 ||
				elfHeader->e_ident[EI_MAG3] != ELFMAG3)
				kpanic(nullptr, nullptr, kpanic_format("Debug info for the kernel is incorrect!"));
			if (elfHeader->e_version == EV_NONE)
				kpanic(nullptr, nullptr, kpanic_format("Debug info for the kernel is incorrect!"));
			if (elfHeader->e_ident[EI_CLASS] != ELFCLASS32 || elfHeader->e_ident[EI_DATA] != ELFDATA2LSB)
				kpanic(nullptr, nullptr, kpanic_format("Debug info for the kernel is incorrect!"));
			Elf32_Shdr* currentSection = reinterpret_cast<Elf32_Shdr*>(startAddress + elfHeader->e_shoff);
			for (int sec = 0; sec < elfHeader->e_shnum; sec++, currentSection++)
			{
				if (!utils::strcmp(getElfString(elfHeader, currentSection->sh_name), ".debug_line"))
					break; // We've found it.
			}
			const DebugLineHeader* lineHeader = reinterpret_cast<DebugLineHeader*>(startAddress + currentSection->sh_offset);
			struct _reg
			{
				UINTPTR_T address;
				UINT32_T op_index;
				UINT32_T file;
				UINT32_T line;
				UINT32_T column;
				bool is_stmt;
				bool basic_block;
				bool end_sequence;
				bool prologue_end;
				bool epilogue_begin;
				UINT32_T isa;
				UINT32_T discriminator;
			} stateMachineRegisters;
			stateMachineRegisters.address = 0;
			stateMachineRegisters.op_index = 0;
			stateMachineRegisters.file = 1;
			stateMachineRegisters.line = 1;
			stateMachineRegisters.is_stmt = lineHeader->default_is_stmt;
			stateMachineRegisters.basic_block = false;
			stateMachineRegisters.end_sequence = false;
			stateMachineRegisters.prologue_end = false;
			stateMachineRegisters.epilogue_begin = false;
			stateMachineRegisters.isa = 0;
			stateMachineRegisters.discriminator = 0;
			s
		}*/
	}
}