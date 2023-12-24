/*
	oboskrnl/process/process.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>
#include <console.h>

#include <multitasking/thread.h>

#include <multitasking/process/arch.h>

#include <allocators/vmm/vmm.h>

#define MULTIASKING_PROCESS_PROCESS_H_INCLUDED

namespace obos
{
	namespace process
	{
		enum
		{
			PROCESS_MAGIC_NUMBER = 0xCA44C071
		};
		struct Process
		{
			Process() = default;
			struct ProcessList
			{
				Process *head, *tail;
				size_t size;
			};
			uint32_t pid;
			uint32_t exitCode = 0;
			thread::Thread::ThreadList threads;
			uint32_t magicNumber = PROCESS_MAGIC_NUMBER;
			Process* parent;
			ProcessList children;
			Console* console;
			bool isUsermode;
			void* _driverIdentity;
			procContextInfo context;
			Process* prev;
			Process* next;
			Process* prev_child;
			Process* next_child;
			memory::VirtualAllocator vallocator{ this };
		};
		extern Process::ProcessList g_processes;
		Process* CreateProcess(bool isUsermode);
	}
}