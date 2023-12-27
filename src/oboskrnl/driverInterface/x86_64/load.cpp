/*
	driverInterface/x86_64/load.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <limine.h>

#include <int.h>
#include <klog.h>
#include <error.h>
#include <memory_manipulation.h>

#include <multitasking/scheduler.h>
#include <multitasking/arch.h>
#include <multitasking/cpu_local.h>

#include <multitasking/threadAPI/thrHandle.h>

#include <driverInterface/load.h>
#include <driverInterface/struct.h>

#include <multitasking/process/x86_64/loader/elfStructures.h>

#include <multitasking/process/process.h>

#include <vector.h>

#define getCPULocal() ((thread::cpu_local*)thread::getCurrentCpuLocalPtr())

namespace obos
{
	extern volatile limine_kernel_file_request kernel_file;
	extern volatile limine_module_request module_request;
	namespace driverInterface
	{
		static const char* getElfString(process::loader::Elf64_Ehdr* elfHeader, uintptr_t index)
		{
			const char* startAddress = reinterpret_cast<const char*>(elfHeader);
			process::loader::Elf64_Shdr* stringTable = reinterpret_cast<process::loader::Elf64_Shdr*>(const_cast<char*>(startAddress) + elfHeader->e_shoff);
			stringTable += elfHeader->e_shstrndx;
			return startAddress + stringTable->sh_offset + index;
		}
		static bool CheckModuleExecutable(byte* file, size_t size)
		{
			using namespace process::loader;
			if (size < sizeof(Elf64_Ehdr))
				return false;
			Elf64_Ehdr* elfHeader = (Elf64_Ehdr*)file;
			if (elfHeader->e_ident[EI_MAG0] != ELFMAG0 ||
				elfHeader->e_ident[EI_MAG1] != ELFMAG1 ||
				elfHeader->e_ident[EI_MAG2] != ELFMAG2 ||
				elfHeader->e_ident[EI_MAG3] != ELFMAG3)
			{
				SetLastError(OBOS_ERROR_LOADER_INCORRECT_FILE);
				return false;
			}
			if (elfHeader->e_version != EV_CURRENT)
			{
				SetLastError(OBOS_ERROR_LOADER_INCORRECT_FILE);
				return false;
			}
			if (elfHeader->e_ident[EI_CLASS] != ELFCLASS64 || elfHeader->e_ident[EI_DATA] != ELFDATA2LSB)
			{
				SetLastError(OBOS_ERROR_LOADER_INCORRECT_ARCHITECTURE);
				return false;
			}
			if (elfHeader->e_type != ET_DYN)
			{
				SetLastError(OBOS_ERROR_LOADER_INCORRECT_FILE);
				return false;
			}
			return true;
		}
		static driverHeader* CheckModule(byte* file, size_t size)
		{
			using namespace process::loader;

			if (!CheckModuleExecutable(file, size))
				return nullptr;
			
			Elf64_Ehdr* elfHeader = (Elf64_Ehdr*)file;
			process::loader::Elf64_Shdr* iter = reinterpret_cast<process::loader::Elf64_Shdr*>(file + elfHeader->e_shoff);
			process::loader::Elf64_Shdr* currentSection = nullptr;

			for (size_t i = 0; i < elfHeader->e_shnum; i++, iter++)
			{
				if (utils::strcmp(OBOS_DRIVER_HEADER_SECTION_NAME, getElfString(elfHeader, iter->sh_name)))
				{
					currentSection = iter;
					break;
				}
			}

			if (!currentSection || !currentSection->sh_offset)
			{
				SetLastError(OBOS_ERROR_NOT_A_DRIVER);
				return nullptr;
			}
			driverHeader* header = reinterpret_cast<driverHeader*>(file + currentSection->sh_offset);
			if (header->magicNumber != OBOS_DRIVER_HEADER_MAGIC)
			{
				SetLastError(OBOS_ERROR_NOT_A_DRIVER);
				return nullptr;
			}
			return header;
		}
		// From https://docs.oracle.com/cd/E23824_01/html/819-0690/chapter6-48031.html#scrolltoc
		static uint32_t ElfHash(const char* name)
		{
			uint32_t h = 0, g = 0;

			while (*name)
			{
				h = (h << 4) + *name++;
				if ((g = h & 0xf0000000))
					h ^= g >> 24;
				h &= ~g;
			}
			return h;
		}
		static process::loader::Elf64_Sym* GetSymbolFromTable(
			byte* fileStart,
			byte* baseAddress,
			process::loader::Elf64_Sym* symbolTable,
			uintptr_t hashTableOff,
			process::loader::Elf64_Off stringTable,
			const char* _symbol)
		{
			using namespace process::loader;
			Elf64_Word* hashTableBase = (Elf64_Word*)(baseAddress + hashTableOff);
			auto& nBuckets = hashTableBase[0];
			auto currentBucket = ElfHash(_symbol) % nBuckets;
			Elf64_Word* firstBucket = hashTableBase + 2;
			Elf64_Word* firstChain = firstBucket + nBuckets;
			size_t index = firstBucket[currentBucket];
			while (index)
			{
				auto& symbol = symbolTable[index];
				const char* symbolName = (char*)(fileStart + stringTable + symbol.st_name);
				if (utils::strcmp(symbolName, _symbol))
					return &symbol;
				
				index = firstChain[index];
			}
			return nullptr;
		}
		static process::loader::Elf64_Sym* GetSymbolFromTable(
			byte* fileStart,
			process::loader::Elf64_Sym* symbolTable,
			size_t szSymbolTable,
			process::loader::Elf64_Off stringTable,
			const char* _symbol)
		{
			using namespace process::loader;
			size_t countEntries = szSymbolTable / sizeof(Elf64_Sym);
			for (size_t i = 0; i < countEntries; i++)
			{
				auto& symbol = symbolTable[i];
				const char* symbolName = (char*)(fileStart + stringTable + symbol.st_name);
				if (utils::strcmp(symbolName, _symbol))
					return &symbol;
			}
			return nullptr;
		}
		struct tables
		{
			process::loader::Elf64_Shdr* symtab_section;
			process::loader::Elf64_Shdr* strtab_section;
		};
		static tables GetKernelSymbolStringTables()
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
			return { symtab_section,strtab_section };
		}
		static process::loader::Elf64_Sym* GetSymbolFromIndex(
			process::loader::Elf64_Sym* symbolTable,
			size_t index)
		{
			return symbolTable + index;
		}
		static bool LoadModuleAndRelocate(byte* file, [[maybe_unused]] size_t size, uintptr_t& _baseAddress, uintptr_t& entry)
		{
			// Load the program headers.
			using namespace process::loader;
			Elf64_Ehdr* elfHeader = (Elf64_Ehdr*)file;
			Elf64_Phdr* programHeaderStart = (Elf64_Phdr*)(file + elfHeader->e_phoff);
			Elf64_Phdr& programHeader = programHeaderStart[0];
			Elf64_Dyn* dynamicHeader = nullptr;
			process::Process* moduleProcess = (process::Process*)thread::GetCurrentCpuLocalPtr()->idleThread->owner;
			memory::VirtualAllocator& vallocator = moduleProcess->vallocator;
			size_t nPagesTotal = 0;
			for (size_t i = 0; i < elfHeader->e_phnum; programHeader = programHeaderStart[i++])
			{
				if (programHeader.p_type != PT_LOAD)
					continue;
				uint32_t nPages = programHeader.p_memsz >> 12;
				if ((programHeader.p_memsz % 4096) != 0)
					nPages++;
				nPagesTotal += nPages;
			}
			// Load the program into memory.
			byte* baseAddress = (byte*)vallocator.VirtualAlloc(nullptr, nPagesTotal * 4096, 0);
			entry = (uintptr_t)(baseAddress + elfHeader->e_entry);
			_baseAddress = (uintptr_t)baseAddress;
			programHeader = *programHeaderStart;
			for (size_t i = 0; i < elfHeader->e_phnum; programHeader = programHeaderStart[i++])
			{
				if (programHeader.p_type == PT_DYNAMIC)
					dynamicHeader = (Elf64_Dyn*)(file + programHeader.p_offset);
				if (programHeader.p_type != PT_LOAD)
					continue;
				if (programHeader.p_filesz)
					utils::memcpy(baseAddress + programHeader.p_vaddr, file + programHeader.p_offset, programHeader.p_filesz);
				else
				{
					// We use "(uintptr_t)(baseAddress + programHeader.p_vaddr) & 0xfff" instead of programHeader.p_memsz because if we zero out the entire region
					// demand paging would be rendered useless.
					utils::memzero(baseAddress + programHeader.p_vaddr, (uintptr_t)(baseAddress + programHeader.p_vaddr) & 0xfff);
				}
			}
			// Apply relocations.
			struct relocation
			{
				uint32_t symbolTableOffset = 0;
				uintptr_t virtualAddress = 0;
				uint16_t relocationType = 0;
				int64_t addend = 0;
			};
			Vector<relocation> required_relocations;
			Elf64_Dyn* currentDynamicHeader = dynamicHeader;
			size_t last_dtrelasz = 0, last_dtrelsz = 0, last_dtpltrelsz = 0;
			uint64_t last_dlpltrel = 0;
			auto handleDtRel = [&](size_t sz) {
				Elf64_Rela* relTable = (Elf64_Rela*)(file + currentDynamicHeader->d_un.d_ptr);
				for (size_t i = 0; i < sz / sizeof(Elf64_Rela); i++)
					required_relocations.push_back({
						(uint32_t)(relTable[i].r_info >> 32),
						relTable[i].r_offset,
						(uint16_t)(relTable[i].r_info & 0xffff),
						relTable[i].r_addend,
						});
				};
			auto handleDtRela = [&](size_t sz) {
				Elf64_Rela* relTable = (Elf64_Rela*)(file + currentDynamicHeader->d_un.d_ptr);
				for (size_t i = 0; i < (sz / sizeof(Elf64_Rela)); i++)
					required_relocations.push_back({
						(uint32_t)(relTable[i].r_info >> 32),
						relTable[i].r_offset,
						(uint16_t)(relTable[i].r_info & 0xffff),
						relTable[i].r_addend,
						});
				};
			[[maybe_unused]] Elf64_Sym*    symbolTable = 0;
			[[maybe_unused]] Elf64_Off     stringTable = 0;
			[[maybe_unused]] uintptr_t hashTableOffset = 0;
			[[maybe_unused]] Elf64_Addr* GOT = nullptr;
			for (size_t i = 0; currentDynamicHeader->d_tag != DT_NULL; i++, currentDynamicHeader++)
			{
				switch (currentDynamicHeader->d_tag)
				{
				case DT_HASH:
					hashTableOffset = currentDynamicHeader->d_un.d_ptr;
					break;
				case DT_PLTGOT:
					// TODO: Find out whether this is the PLT or GOT (if possible).
					GOT = (Elf64_Addr*)(baseAddress + currentDynamicHeader->d_un.d_ptr);
					break;
				case DT_REL:
					handleDtRel(last_dtrelsz);
					break;
				case DT_RELA:
					handleDtRela(last_dtrelasz);
					break;
				case DT_JMPREL:
					switch (last_dlpltrel)
					{
					case DT_REL:
						handleDtRel(last_dtpltrelsz);
						break;
					case DT_RELA:
						handleDtRela(last_dtpltrelsz);
						break;
					default:
						break;
					}
					break;
				case DT_RELSZ:
					last_dtrelsz = currentDynamicHeader->d_un.d_val;
					break;
				case DT_RELASZ:
					last_dtrelasz = currentDynamicHeader->d_un.d_val;
					break;
				case DT_PLTREL:
					last_dlpltrel = currentDynamicHeader->d_un.d_val;
					break;
				case DT_PLTRELSZ:
					last_dtpltrelsz = currentDynamicHeader->d_un.d_val;
					break;
				case DT_STRTAB:
					stringTable = currentDynamicHeader->d_un.d_ptr;
					break;
				case DT_SYMTAB:
					symbolTable = (Elf64_Sym*)(baseAddress + currentDynamicHeader->d_un.d_ptr);
					break;
				default:
					break;
				}
			}
			byte* kFileBase = (byte*)kernel_file.response->kernel_file->address;
			tables& kernel_tables = GetKernelSymbolStringTables();
			for ([[maybe_unused]] const relocation& i : required_relocations)
			{
				[[maybe_unused]] auto& Unresolved_Symbol = *GetSymbolFromIndex(symbolTable, i.symbolTableOffset);
				[[maybe_unused]] auto Symbol = GetSymbolFromTable(
					kFileBase,
					(Elf64_Sym*)(kFileBase + kernel_tables.symtab_section->sh_offset),
					kernel_tables.symtab_section->sh_size,
					kernel_tables.strtab_section->sh_offset,
					(const char*)(baseAddress + stringTable + Unresolved_Symbol.st_name)
				);
				if (!Symbol)
				{
					SetLastError(OBOS_ERROR_DRIVER_REFERENCED_UNRESOLVED_SYMBOL);
					break;
				}
				auto& type = i.relocationType;
				uintptr_t relocAddr = (uintptr_t)baseAddress + i.virtualAddress;
				uint64_t relocResult = 0;
				uint8_t relocSize = 0;
				switch (type)
				{
				case R_AMD64_NONE:
					continue;
				case R_AMD64_64:
					relocResult = Symbol->st_value + i.addend;
					relocSize = 8;
					break;
				case R_AMD64_PC32:
					relocResult = (Symbol->st_value + i.addend - relocAddr) & 0xffffffff;
					relocSize = 4;
					break;
				case R_AMD64_GOT32:
					// TODO: Implement.
					relocResult = (0 + i.addend) & 0xffffffff;
					relocSize = 4;
					break;
				}
			}
			// Apply protection flags.
			programHeader = *programHeaderStart;
			for (size_t i = 0; i < elfHeader->e_phnum; programHeader = programHeaderStart[i++])
			{
				if (programHeader.p_type != PT_LOAD)
					continue;
				uintptr_t flags = 0;
				if ((programHeader.p_flags & PF_R) && !(programHeader.p_flags & PF_W))
					flags |= memory::PROT_READ_ONLY;
				if (programHeader.p_flags & PF_X)
					flags |= memory::PROT_CAN_EXECUTE;
				vallocator.VirtualProtect(baseAddress + programHeader.p_vaddr, programHeader.p_memsz, flags);
			}
			return true;
		}
		bool LoadModule(byte* file, size_t size, thread::ThreadHandle** mainThread)
		{
			driverHeader* header = CheckModule(file, size);
			if (!header)
				return false;

			driverIdentity* identity = new driverIdentity;
			identity->driverId = header->driverId;
			identity->_serviceType = header->driverType;

			if (header->requests & driverHeader::VPRINTF_FUNCTION_REQUEST)
				header->vprintfFunctionResponse = logger::vprintf;
			if (header->requests & driverHeader::PANIC_FUNCTION_REQUEST)
				header->panicFunctionResponse = logger::panicVariadic;
			if (header->requests & driverHeader::MEMORY_MANIPULATION_FUNCTIONS_REQUEST)
			{
				header->memoryManipFunctionsResponse.memzero = utils::memzero;
				header->memoryManipFunctionsResponse.memcpy = utils::memcpy;
				header->memoryManipFunctionsResponse.memcmp = utils::memcmp;
				header->memoryManipFunctionsResponse.memcmp_toByte = utils::memcmp;
				header->memoryManipFunctionsResponse.strlen = utils::strlen;
				header->memoryManipFunctionsResponse.strcmp = utils::strcmp;
			}
			if (header->requests & driverHeader::INITRD_LOCATION_REQUEST)
			{
				for (size_t moduleIndex = 0; moduleIndex < module_request.response->module_count; moduleIndex++)
				{
					if (utils::strcmp(module_request.response->modules[moduleIndex]->path, "/obos/initrd.tar"))
					{
						header->initrdLocationResponse.addr = module_request.response->modules[moduleIndex]->address;
						header->initrdLocationResponse.size = module_request.response->modules[moduleIndex]->size;
						break;
					}
				}
			}
			
			uintptr_t entryPoint = 0, baseAddress = 0;
			if (!LoadModuleAndRelocate(file, size, baseAddress, entryPoint))
				return false;

			thread::ThreadHandle* thread = new thread::ThreadHandle{};
			if (mainThread)
				*mainThread = thread;

			thread->CreateThread(
				thread::THREAD_PRIORITY_NORMAL, 
				(header->requests & driverHeader::SET_STACK_SIZE_REQUEST) ? header->stackSize : 0,
				(void(*)(uintptr_t))entryPoint,
				0,
				thread::g_defaultAffinity,
				thread::GetCurrentCpuLocalPtr()->idleThread->owner, // The idle thread always uses the kernel's process as it's owner.
				false);
			
			return true;
		}
	}
}