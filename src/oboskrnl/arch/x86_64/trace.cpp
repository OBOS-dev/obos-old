/*
	oboskrnl/arch/x86_64/trace.cpp
	
	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <limine.h>
#include <memory_manipulation.h>

#include <allocators/vmm/vmm.h>

#include <multitasking/process/x86_64/loader/elfStructures.h>

#include <driverInterface/struct.h>
#include <driverInterface/load.h>

#include <x86_64-utils/asm.h>

#include <arch/interrupt.h>

#define CR4_SMAP ((uintptr_t)1<<21)

namespace obos
{
	extern volatile limine_module_request module_request;
	volatile limine_kernel_file_request kernel_file = {
		.id = LIMINE_KERNEL_FILE_REQUEST,
		.revision = 1,
	};
	namespace logger
	{
		static const char* getElfStringFromSection(process::loader::Elf64_Ehdr* elfHeader, process::loader::Elf64_Shdr* section, uintptr_t index)
		{
			const char* startAddress = reinterpret_cast<const char*>(elfHeader);
			return startAddress + section->sh_offset + index;
		}
		static const char* getElfString(process::loader::Elf64_Ehdr* elfHeader, uintptr_t index)
		{
			process::loader::Elf64_Shdr* stringTable = reinterpret_cast<process::loader::Elf64_Shdr*>(reinterpret_cast<char*>(elfHeader) + elfHeader->e_shoff);
			stringTable += elfHeader->e_shstrndx;
			return getElfStringFromSection(elfHeader, stringTable, index);
		}
		void addr2func(void* addr, const char*& str, size_t& functionAddress)
		{
			uintptr_t kernelFileStart = (uintptr_t)kernel_file.response->kernel_file->address;
			process::loader::Elf64_Ehdr* eheader = (process::loader::Elf64_Ehdr*)kernelFileStart;
			process::loader::Elf64_Shdr* iter = reinterpret_cast<process::loader::Elf64_Shdr*>(kernelFileStart + eheader->e_shoff);
			process::loader::Elf64_Shdr* symtab_section = nullptr;
			process::loader::Elf64_Shdr* strtab_section = nullptr;

			for (size_t i = 0; i < eheader->e_shnum; i++, iter++)
			{
				const char* section_name = getElfString(eheader, iter->sh_name);
				if (utils::strcmp(".symtab", section_name))
				{
					symtab_section = iter;
					continue;
				}
				if (utils::strcmp(".strtab", section_name))
				{
					strtab_section = iter;
					continue;
				}
				if (strtab_section != nullptr && symtab_section != nullptr)
					break;
			}
			if (!symtab_section)
				return;
			if (!strtab_section)
				return;
			uintptr_t _addr = (uintptr_t)addr;
			process::loader::Elf64_Sym* symbol_table = reinterpret_cast<process::loader::Elf64_Sym*>(kernelFileStart + symtab_section->sh_offset);
			process::loader::Elf64_Sym* sIter = symbol_table;
			process::loader::Elf64_Sym* symbol = nullptr;
			for (; (uintptr_t)sIter < ((uintptr_t)symbol_table + symtab_section->sh_size); sIter++)
			{
				if (_addr >= sIter->st_value && _addr < (sIter->st_value + sIter->st_size))
				{
					symbol = sIter;
					break;
				}
			}
			if (!symbol)
			{
				// Look in the driver symbol tables
				auto searchDriver = [&_addr](driverInterface::driverIdentity* driver)->driverInterface::obosDriverSymbol*
					{
						if (!driver)
							return nullptr;
						for (auto& sym : driver->symbols)
							if (_addr >= sym.addr && _addr < (sym.addr + sym.size))
								return &sym;
						return nullptr;
					};
				driverInterface::obosDriverSymbol* symbol = nullptr;
				for (auto iter = driverInterface::g_driverInterfaces.begin(); iter; iter++)
				{
					if (!(iter))
						continue;
					if (!(*iter).value)
						continue;
					if ((symbol = searchDriver(*(*iter).value)))
						break;
				}
				if (!symbol)
					return;
				str = symbol->name;
				functionAddress = symbol->addr;
				return;
			}
			str = getElfStringFromSection(eheader, strtab_section, symbol->st_name);
			functionAddress = symbol->st_value;
		}

		struct stack_frame
		{
			stack_frame* down;
			void* rip;
		};
		void stackTrace(void* parameter)
		{
			memory::VirtualAllocator valloc{ nullptr };
			if (getCR4() & CR4_SMAP)
				restorePreviousInterruptStatus(getEflags() | x86_64_flags::RFLAGS_ALIGN_CHECK); // Ensure that we won't page fault while accessing a user stack
			volatile stack_frame* frame = parameter ? (volatile stack_frame*)parameter : (volatile stack_frame*)__builtin_frame_address(0);
			{
				uintptr_t attrib = 0;
				valloc.VirtualGetProtection((void*)frame, 1, &attrib);
				if (!attrib)
					frame = (void*)frame == parameter ? (volatile stack_frame*)__builtin_frame_address(0) : (volatile stack_frame*)parameter;
			}
			int nFrames = -1;
			for (; frame; nFrames++) 
			{
				frame = frame->down;
				if(frame)
				{
					if (!frame->down)
						continue;
					uintptr_t attrib = 0;
					valloc.VirtualGetProtection((void*)frame->down, 1, &attrib);
					if (!attrib)
					{
						nFrames++;
						break;
					}
				}
				else
					break;
			}
			{
				frame = parameter ? (volatile stack_frame*)parameter : (volatile stack_frame*)__builtin_frame_address(0);
				uintptr_t attrib = 0;
				valloc.VirtualGetProtection((void*)frame, 1, &attrib);
				if (!attrib)
					frame = (void*)frame == parameter ? (volatile stack_frame*)__builtin_frame_address(0) : (volatile stack_frame*)parameter;
			}
			printf("Stack trace:\n");
			for (int i = nFrames; i != -1; i--)
			{
				const char* functionName = nullptr;
				uintptr_t functionStart = 0;
				if (frame->rip > (void*)0xffffffff00000000)
					addr2func(frame->rip, functionName, functionStart);
				if(functionName)
					printf("\t0x%p: %s+%d\n", frame->rip, functionName, (uintptr_t)frame->rip - functionStart);
				else
					printf("\t0x%p: External Code\n", frame->rip);
				frame = frame->down;
			}
		}
	}
}