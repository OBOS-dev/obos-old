/*
	process.h

	Copyright (c) 2023 oboskrnl/process/process.h
*/

#pragma once

#include <types.h>

#include <memory_manager/paging/init.h>

#include <external/list/list.h>

namespace obos
{
	namespace process
	{
		VOID DriverEntryPoint(PVOID entry);
		extern DWORD g_nextProcessId;
		extern list_t* g_processList;
		extern UINTPTR_T ProcEntryPointBase;
		class Process
		{
		public:
			Process();

			/// <summary>
			/// Creates the process from an elf file's data.
			/// </summary>
			/// <param name="file">A pointer to the complete elf file's data.</param>
			/// <param name="size">The size of the buffer.</param>
			/// <param name="mainThread">A pointer to an obos::multitasking::ThreadHandle to be opened with the process's main thread.</param>
			/// <returns>Success is zero. The elf file's exit status if it's from 1-3, and the elf loader failed.
			/// If the return value is 4 and up, it is CreateThread's exit status, if it failed.
			/// 0xFFFFFFFF if a process exists in the current object.</returns>
			DWORD CreateProcess(PBYTE file, SIZE_T size, PVOID mainThread, bool isDriver = false);
			/// <summary>
			/// Terminates the process.
			/// </summary>
			/// <param name="exitCode">The process's exit code.</param>
			/// <returns>Success is zero, or 0xFFFFFFFF if there is no process.</returns>
			DWORD TerminateProcess(DWORD exitCode, bool canExitThread = true);

			// Make sure the stack is in a good place before running.
			void doContextSwitch();

			Process(Process&&) = delete;
			Process(const Process&) = delete;

			Process& operator=(const Process&) = delete;
			Process& operator=(Process&&) = delete;

			~Process();
		public:
#if defined (__i686__)
			memory::PageDirectory* pageDirectory = nullptr;
#elif defined(__x86_64__)
			memory::PageMap* level4PageMap = nullptr;
#endif
			DWORD pid = 0;
			Process* parent = nullptr;
			list_t* children = nullptr;
			list_t* threads = nullptr;
			list_t* abstractHandles = nullptr;
			struct allocatedBlock
			{
				PBYTE start = nullptr;
				SIZE_T size = 0;
			};
			// a list of "address"
			list_t* allocatedBlocks;
			bool isUserMode = false;
			DWORD exitCode = 0;
			DWORD consoleForegroundColour;
			DWORD consoleBackgroundColour;
			UINTPTR_T magicNumber = 0xCA44C071;
		private:
			void UncommitProcessMemory();
		};
	}
}