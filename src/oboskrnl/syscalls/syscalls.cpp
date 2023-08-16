/*
	oboskrnl/syscalls/syscalls.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <syscalls/syscalls.h>

#include <utils/memory.h>

#include <console.h>

#include <multitasking/multitasking.h>

extern int do_nothing();

//#define DEFINE_RESERVED_PARAMETERS volatile UINTPTR_T, volatile UINTPTR_T, volatile UINTPTR_T, volatile UINTPTR_T, volatile UINTPTR_T

namespace obos
{
	namespace process
	{
		static void ConsoleOutputString(CSTRING string);
		static void ExitProcess();

		UINTPTR_T g_syscallTable[512];
		void RegisterSyscalls()
		{
			utils::dwMemset(g_syscallTable, (DWORD)do_nothing, sizeof(g_syscallTable) / sizeof(g_syscallTable[0]));
			DWORD nextSyscall = 0;
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(ConsoleOutputString);
			g_syscallTable[nextSyscall++] = GET_FUNC_ADDR(ExitProcess);
		}
	
		static void ConsoleOutputString(CSTRING string)
		{
			obos::ConsoleOutputString(string);
		}
		static void ExitProcess()
		{
			multitasking::g_currentThread->owner->TerminateProcess();
		}
	}
}