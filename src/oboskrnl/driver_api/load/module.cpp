/*
	oboskrnl/driver_api/load/module.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>
#include <error.h>
#include <inline-asm.h>

#include <driver_api/load/module.h>
#include <driver_api/syscalls.h>
#include <driver_api/enums.h>

#include <elf/elf.h>
#include <elf/elfStructures.h>

#include <utils/memory.h>

#include <multitasking/threadHandle.h>

namespace obos
{
	namespace driverAPI
	{
		static CSTRING getElfString(elfLoader::Elf64_Ehdr* elfHeader, DWORD index)
		{
			CSTRING startAddress = reinterpret_cast<CSTRING>(elfHeader);
			elfLoader::Elf64_Shdr* stringTable = reinterpret_cast<elfLoader::Elf64_Shdr*>(const_cast<STRING>(startAddress) + elfHeader->e_shoff);
			stringTable += elfHeader->e_shstrndx;
			return startAddress + stringTable->sh_offset + index;
		}

		driverIdentification* LoadModule(PBYTE file, SIZE_T size, multitasking::ThreadHandle* _mainThreadHandle)
		{
			if (!file || !size)
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return nullptr;
			}

			// Verify the file.
			if (elfLoader::CheckElfFile(file, size, true) != 0)
				return nullptr;

			// Read the driver's header.
			elfLoader::Elf64_Ehdr* elfHeader = (elfLoader::Elf64_Ehdr*)file;
			elfLoader::Elf64_Shdr* currentSection = reinterpret_cast<elfLoader::Elf64_Shdr*>(file + elfHeader->e_shoff);

			for (SIZE_T i = 0; i < elfHeader->e_shnum; i++, currentSection++)
			{
				if (!utils::strcmp(OBOS_DRIVER_HEADER_SECTION, getElfString(elfHeader, currentSection->sh_name)))
					break;
			}

			if (utils::strcmp(OBOS_DRIVER_HEADER_SECTION, getElfString(elfHeader, currentSection->sh_name)) != 0 || !currentSection->sh_offset)
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

			driverIdentification* identification = new driverIdentification{};
			identification->driverId = header->driverId;
			identification->service_type = header->service_type;

			EnterKernelSection();

			process::Process* driverProc = new process::Process{};

			multitasking::ThreadHandle mainThread;
			driverProc->CreateProcess(file, size, &mainThread, true);
			mainThread.PauseThread();

			LeaveKernelSection();

			identification->process = driverProc;
			
			if (identification->driverId > g_registeredDriversCapacity)
			{
				g_registeredDriversCapacity = identification->driverId + (identification->driverId % 4);
				g_registeredDrivers = (driverIdentification**)krealloc(g_registeredDrivers, g_registeredDriversCapacity);
			}

			g_registeredDrivers[identification->driverId] = identification;

			mainThread.ResumeThread();

			if (_mainThreadHandle)
				_mainThreadHandle->OpenThread((multitasking::Thread*)mainThread.GetUnderlyingObject());

			return identification;
		}
	}
}