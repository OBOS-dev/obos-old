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
		extern DWORD g_nextProcessId;
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
			/// <returns>Success is zero. The elf file's exit status is it's from 1-3. Four if the function could not create the main thread. 0xFFFFFFFF if a process exists in
			/// the current object.</returns>
			DWORD CreateProcess(PBYTE file, SIZE_T size, PVOID mainThread, bool isDriver = false);
			/// <summary>
			/// Terminates the process.
			/// </summary>
			/// <returns>Success is zero, or 0xFFFFFFFF if there is no process.</returns>
			DWORD TerminateProcess(void);

			Process(Process&&) = delete;
			Process(const Process&) = delete;

			Process& operator=(const Process&) = delete;
			Process& operator=(Process&&) = delete;

			~Process();
		public:
			memory::PageDirectory* pageDirectory = nullptr;
			DWORD pid = 0;
			Process* parent = nullptr;
			list_t* children = nullptr;
			list_t* threads = nullptr;
			list_t* mutexes = nullptr;
		};
	}
}