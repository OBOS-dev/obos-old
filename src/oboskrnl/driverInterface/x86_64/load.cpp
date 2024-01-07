/*
	driverInterface/x86_64/load.cpp

	Copyright (c) 2023-2024 Omar Berrow
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
		driverHeader* _CheckModuleImpl(byte* file, size_t size, uintptr_t* headerVirtAddr)
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
			if(headerVirtAddr)
				*headerVirtAddr = currentSection->sh_addr;
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
		process::loader::Elf64_Sym* GetSymbolFromTable(
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
			const char* _symbol,
			uint8_t requiredBinding = process::loader::STB_GLOBAL)
		{
			using namespace process::loader;
			size_t countEntries = szSymbolTable / sizeof(Elf64_Sym);
			for (size_t i = 0; i < countEntries; i++)
			{
				auto& symbol = symbolTable[i];
				if ((symbol.st_info >> 4) != requiredBinding)
					continue;
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
				return { nullptr,nullptr };
			if (!strtab_section)
				return { nullptr,nullptr };
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
			utils::Vector<relocation> required_relocations;
			Elf64_Dyn* currentDynamicHeader = dynamicHeader;
			size_t last_dtrelasz = 0, last_dtrelsz = 0, last_dtpltrelsz = 0;
			bool awaitingRelaSz = false, foundRelaSz = false;
			Elf64_Dyn* dynEntryAwaitingRelaSz = nullptr;
			bool awaitingRelSz = false, foundRelSz = false;
			Elf64_Dyn* dynEntryAwaitingRelSz = nullptr;
			uint64_t last_dlpltrel = 0;
			auto handleDtRel = [&](Elf64_Dyn* dynamicHeader, size_t sz) {
				Elf64_Rela* relTable = (Elf64_Rela*)(file + dynamicHeader->d_un.d_ptr);
				for (size_t i = 0; i < sz / sizeof(Elf64_Rela); i++)
					required_relocations.push_back({
						(uint32_t)(relTable[i].r_info >> 32),
						relTable[i].r_offset,
						(uint16_t)(relTable[i].r_info & 0xffff),
						relTable[i].r_addend,
						});
				};
			auto handleDtRela = [&](Elf64_Dyn* dynamicHeader, size_t sz) {
				Elf64_Rela* relTable = (Elf64_Rela*)(file + dynamicHeader->d_un.d_ptr);
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
					if (!foundRelSz)
					{
						awaitingRelSz = true;
						dynEntryAwaitingRelSz = currentDynamicHeader;
						break;
					}
					handleDtRel(currentDynamicHeader, last_dtrelsz);
					foundRelSz = false;
					last_dtrelsz = 0;
					break;
				case DT_RELA:
					if (!foundRelaSz)
					{
						awaitingRelaSz = true;
						dynEntryAwaitingRelaSz = currentDynamicHeader;
						break;
					}
					handleDtRela(currentDynamicHeader, last_dtrelasz);
					foundRelaSz = false;
					last_dtrelasz = 0;
					break;
				case DT_JMPREL:
					switch (last_dlpltrel)
					{
					case DT_REL:
						handleDtRel(currentDynamicHeader, last_dtpltrelsz);
						break;
					case DT_RELA:
						handleDtRela(currentDynamicHeader, last_dtpltrelsz);
						break;
					default:
						break;
					}
					break;
				case DT_RELSZ:
					last_dtrelsz = currentDynamicHeader->d_un.d_val;
					foundRelSz = !awaitingRelSz;
					if (awaitingRelSz)
					{
						handleDtRel(dynEntryAwaitingRelSz, last_dtrelsz);
						awaitingRelSz = false;
					}
					break;
				case DT_RELASZ:
					last_dtrelasz = currentDynamicHeader->d_un.d_val;
					foundRelaSz = !awaitingRelaSz;
					if (awaitingRelaSz)
					{
						handleDtRel(dynEntryAwaitingRelaSz, last_dtrelasz);
						awaitingRelaSz = false;
					}
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
			struct copy_reloc
			{
				void* src = nullptr, *dest = nullptr;
				size_t size = 0;
			};
			utils::Vector<copy_reloc> copy_relocations;
			byte* kFileBase = (byte*)kernel_file.response->kernel_file->address;
			tables kernel_tables = GetKernelSymbolStringTables();
			for ([[maybe_unused]] const relocation& i : required_relocations)
			{
				Elf64_Sym* Symbol = nullptr;
				if (i.symbolTableOffset)
				{
					auto& Unresolved_Symbol = *GetSymbolFromIndex(symbolTable, i.symbolTableOffset);
					Symbol = GetSymbolFromTable(
						kFileBase,
						(Elf64_Sym*)(kFileBase + kernel_tables.symtab_section->sh_offset),
						kernel_tables.symtab_section->sh_size,
						kernel_tables.strtab_section->sh_offset,
						(const char*)(baseAddress + stringTable + Unresolved_Symbol.st_name)
					);
					if (!Symbol) 
					{
						SetLastError(OBOS_ERROR_DRIVER_REFERENCED_UNRESOLVED_SYMBOL);
						vallocator.VirtualFree(baseAddress, nPagesTotal);
						return false;
					}
					if (Unresolved_Symbol.st_size != Symbol->st_size && i.relocationType == R_AMD64_COPY)
					{
						// Oh no!
						SetLastError(OBOS_ERROR_DRIVER_SYMBOL_MISMATCH);
						vallocator.VirtualFree(baseAddress, nPagesTotal);
						return false;
					}
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
					relocResult = Symbol->st_value + i.addend - relocAddr;
					relocSize = 4;
					break;
				case R_AMD64_GOT32:
					SetLastError(OBOS_ERROR_UNIMPLEMENTED_FEATURE);
					vallocator.VirtualFree(baseAddress, nPagesTotal);
					return false;
					// TODO: Replace the zero in the calculation with "G" (see elf spec for more info).
					relocResult = 0 + i.addend;
					relocSize = 4;
					break;
				case R_AMD64_PLT32:
					SetLastError(OBOS_ERROR_UNIMPLEMENTED_FEATURE);
					vallocator.VirtualFree(baseAddress, nPagesTotal);
					return false;
					// TODO: Replace the zero in the calculation with "L" (see elf spec for more info).
					relocResult = 0 + i.addend - relocAddr;
					relocSize = 4;
					break;
				case R_AMD64_COPY:
					// Save copy relocations for the end because if we don't, it might contain unresolved addresses.
					copy_relocations.push_back({ (void*)relocAddr, (void*)Symbol->st_value, Symbol->st_size });
					relocSize = 0;
					break;
				case R_AMD64_JUMP_SLOT:
				case R_AMD64_GLOB_DAT:
					relocResult = Symbol->st_value;
					relocSize = 8;
					break;
				case R_AMD64_RELATIVE:
					relocResult = (uint64_t)baseAddress + i.addend;
					relocSize = 8;
					break;
				case R_AMD64_GOTPCREL:
					SetLastError(OBOS_ERROR_UNIMPLEMENTED_FEATURE);
					vallocator.VirtualFree(baseAddress, nPagesTotal);
					return false;
					// TODO: Replace the zero in the calculation with "G" (see elf spec for more info).
					relocResult = 0 + (uint64_t)baseAddress + i.addend - relocAddr;
					relocSize = 8;
					break;
				case R_AMD64_32:
					relocResult = Symbol->st_value + i.addend;
					relocSize = 4;
					break;
				case R_AMD64_32S:
					relocResult = Symbol->st_value + i.addend;
					relocSize = 4;
					break;
				case R_AMD64_16:
					relocResult = Symbol->st_value + i.addend;
					relocSize = 2;
					break;
				case R_AMD64_PC16:
					relocResult = Symbol->st_value + i.addend - relocAddr;
					relocSize = 2;
					break;
				case R_AMD64_8:
					relocResult = Symbol->st_value + i.addend;
					relocSize = 1;
					break;
				case R_AMD64_PC8:
					relocResult = Symbol->st_value + i.addend - relocAddr;
					relocSize = 1;
					break;
				case R_AMD64_PC64:
					relocResult = Symbol->st_value + i.addend - relocAddr;
					relocSize = 8;
					break;
				case R_AMD64_GOTOFF64:
					relocResult = Symbol->st_value + i.addend - ((uint64_t)GOT);
					relocSize = 8;
					break;
				case R_AMD64_GOTPC32:
					relocResult = (uint64_t)GOT + i.addend + relocAddr;
					relocSize = 8;
					break;
				case R_AMD64_SIZE32:
					relocSize = 4;
					relocResult = Symbol->st_size + i.addend;
					break;
				case R_AMD64_SIZE64:
					relocSize = 8;
					relocResult = Symbol->st_size + i.addend;
					break;
				default:
					break; // Ignore.
				}
				switch (relocSize)
				{
				case 0:
					break; // The relocation type is rather unsupported or special.
				case 1: // word8
					*(uint8_t*)(relocAddr) = (uint8_t)(relocResult & 0xff);
					break;
				case 2: // word16
					*(uint16_t*)(relocAddr) = (uint16_t)(relocResult & 0xffff);
					break;
				case 4: // word32
					*(uint32_t*)(relocAddr) = (uint16_t)(relocResult & 0xffffffff);
					break;
				case 8: // word64
					*(uint64_t*)(relocAddr) = relocResult;
					break;
				default:
					break;
				}
			}
			// Apply copy relocations.
			for (auto& reloc : copy_relocations)
				utils::memcpy(reloc.src, reloc.dest, reloc.size);
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
		driverIdentity** g_driverInterfaces;
		size_t g_driverInterfacesCapacity;
		constexpr static size_t g_driverInterfacesCapacityStep = 4;
		driverHeader* CheckModule(byte* file, size_t size)
		{
			return _CheckModuleImpl(file, size, nullptr);
		}
		bool LoadModule(byte* file, size_t size, thread::ThreadHandle** mainThread)
		{
			uintptr_t headerVirtAddr = 0;
			driverHeader* header = _CheckModuleImpl(file, size, &headerVirtAddr);
			if (!header)
				return false;
			if (!header->functionTable.GetServiceType)
			{
				SetLastError(OBOS_ERROR_INVALID_DRIVER_HEADER);
				return false;
			}

			uintptr_t entryPoint = 0, baseAddress = 0;
			if (!LoadModuleAndRelocate(file, size, baseAddress, entryPoint))
				return false;
			
			header = (driverHeader*)(baseAddress + headerVirtAddr);

			driverIdentity* identity = new driverIdentity;
			identity->driverId = header->driverId;
			identity->_serviceType = header->driverType;
			identity->functionTable = header->functionTable;
			identity->header = header;
			// We emplace it after the driver finishes initialization.
			if (identity->driverId >= g_driverInterfacesCapacity)
				g_driverInterfaces = new driverIdentity*[g_driverInterfacesCapacity + g_driverInterfacesCapacityStep];
			if(g_driverInterfaces[identity->driverId])
			{
				SetLastError(OBOS_ERROR_ALREADY_EXISTS);
				return false;
			}
			if (header->requests & driverHeader::REQUEST_INITRD_LOCATION)
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

			if (!(header->requests & driverHeader::REQUEST_NO_MAIN_THREAD))
			{
				thread::ThreadHandle* thread = new thread::ThreadHandle{};
				if (mainThread)
					*mainThread = thread;

				thread->CreateThread(
					thread::THREAD_PRIORITY_NORMAL, 
					(header->requests & driverHeader::REQUEST_SET_STACK_SIZE) ? header->stackSize : 0,
					(void(*)(uintptr_t))entryPoint,
					0,
					// thread::g_defaultAffinity,
					2,
					thread::GetCurrentCpuLocalPtr()->idleThread->owner, // The idle thread always uses the kernel's process as it's owner.
					true);
				((thread::Thread*)thread->GetUnderlyingObject())->driverIdentity = identity;
				thread->ResumeThread();
			}
			else
				header->driver_initialized = true;
			while (!header->driver_initialized);
			g_driverInterfaces[identity->driverId] = identity;
			header->driver_finished_loading = true;
			return true;
		}
	}
}