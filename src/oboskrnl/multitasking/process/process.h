/*
	oboskrnl/process/process.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <multitasking/thread.h>

#include <multitasking/process/arch.h>

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
			bool isUsermode;
			procContextInfo context;
		};
		Process* CreateProcess(bool isUsermode);
	}
}