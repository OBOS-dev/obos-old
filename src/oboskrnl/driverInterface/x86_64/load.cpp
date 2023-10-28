/*
	driverInterface/x86_64/load.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <error.h>
#include <memory_manipulation.h>

#include <multitasking/scheduler.h>
#include <multitasking/threadAPI/thrHandle.h>

#include <driverInterface/load.h>
#include <driverInterface/struct.h>

#include <multitasking/process/x86_64/loader/elf.h>
#include <multitasking/process/x86_64/loader/elfStructures.h>

#include <multitasking/process/process.h>

namespace obos
{
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

			uintptr_t val = thread::stopTimer();
			process::switchToProcessContext(&driverProc->context);

			uintptr_t entryPoint = 0, baseAddress = 0;

			uint32_t ret = process::loader::LoadElfFile(file, size, entryPoint, baseAddress);
			if (ret)
				return 0;

			thread::ThreadHandle* thread = new thread::ThreadHandle{};
			if (mainThread)
				*mainThread = thread;
			thread->CreateThread(thread::THREAD_PRIORITY_NORMAL, header->stackSize, (void(*)(uintptr_t))entryPoint, 0, &driverProc->threads);
			((thread::Thread*)thread->GetUnderlyingObject())->owner = driverProc;

			process::switchToProcessContext(&((process::Process*)thread::g_currentThread->owner)->context);
			thread::startTimer(val);

			return driverProc->pid;
		}
	}
}