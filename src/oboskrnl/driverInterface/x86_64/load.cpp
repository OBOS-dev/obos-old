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

#include <multitasking/process/x86_64/loader/elf.h>
#include <multitasking/process/x86_64/loader/elfStructures.h>

#include <multitasking/process/process.h>

#define getCPULocal() ((thread::cpu_local*)thread::getCurrentCpuLocalPtr())

namespace obos
{
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

		uint32_t LoadModule(byte* file, size_t size, thread::ThreadHandle** mainThread)
		{
			if (process::loader::CheckElfFile(file, size, true))
				return 0;

			process::loader::Elf64_Ehdr* elfHeader = (process::loader::Elf64_Ehdr*)file;
			process::loader::Elf64_Shdr* currentSection = reinterpret_cast<process::loader::Elf64_Shdr*>(file + elfHeader->e_shoff);

			for (size_t i = 0; i < elfHeader->e_shnum; i++, currentSection++)
			{
				if (utils::strcmp(OBOS_DRIVER_HEADER_SECTION_NAME, getElfString(elfHeader, currentSection->sh_name)))
					break;
			}

			if (!utils::strcmp(OBOS_DRIVER_HEADER_SECTION_NAME, getElfString(elfHeader, currentSection->sh_name)) != 0 || !currentSection->sh_offset)
			{
				SetLastError(OBOS_ERROR_NOT_A_DRIVER);
				return 0;
			}

			driverHeader* header = reinterpret_cast<driverHeader*>(file + currentSection->sh_offset);
			if (header->magicNumber != OBOS_DRIVER_HEADER_MAGIC)
			{
				SetLastError(OBOS_ERROR_NOT_A_DRIVER);
				return 0;
			}

			driverIdentity* identity = new driverIdentity;
			identity->driverId = header->driverId;
			identity->_serviceType = header->driverType;

			process::Process* driverProc = process::CreateProcess(false);
			
			identity->process = driverProc;
			driverProc->_driverIdentity = identity;

			uintptr_t entryPoint = 0, baseAddress = 0;
			
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

			thread::ThreadHandle* thread = new thread::ThreadHandle{};
			if (mainThread)
				*mainThread = thread;

			uint32_t ret = process::loader::LoadElfFile(file, size, entryPoint, baseAddress, driverProc->vallocator);
			if (ret)
				return 0;

			thread->CreateThread(
				thread::THREAD_PRIORITY_NORMAL, 
				(header->requests & driverHeader::SET_STACK_SIZE_REQUEST) ? header->stackSize : 0,
				(void(*)(uintptr_t))entryPoint,
				0,
				thread::g_defaultAffinity,
				&driverProc->threads,
				true);
			((thread::Thread*)thread->GetUnderlyingObject())->owner = driverProc;
			((thread::Thread*)thread->GetUnderlyingObject())->context.cr3 = driverProc->context.cr3;
			thread->ResumeThread();

			return driverProc->pid;
		}
	}
}