/*
	oboskrnl/process/arch.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <multitasking/thread.h>

#if defined(__x86_64__) || defined(_WIN64)
#include <multitasking/process/x86_64/procInfo.h>
#endif

namespace obos
{
	namespace process
	{
		void setupContextInfo(procContextInfo* info);
		// Don't free the process' virtual address space, as that's done with virtual allocator in TerminateProcess.
		void freeProcessContext(procContextInfo* info);
		void switchToProcessContext(procContextInfo* info);

		// Makes the thread be at "function" with the current cpu's temporary stack, and passes par1 and par2 to the function.
		[[noreturn]] void putThreadAtFunctionWithCPUTempStack(thread::Thread* thread, void* function, void* par1, void* par2);
	}
}