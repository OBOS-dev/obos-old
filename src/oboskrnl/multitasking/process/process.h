/*
	oboskrnl/process/process.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <console.h>

#include <multitasking/thread.h>

#include <multitasking/process/arch.h>

#include <allocators/vmm/vmm.h>
#include <allocators/slab.h>

#include <multitasking/process/signals.h>

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
			procContextInfo context;
			Process* prev;
			Process* next;
			Process* prev_child;
			Process* next_child;
			memory::VirtualAllocator vallocator{ this };
			void(*signal_table[SIGMAX])();
			void* operator new(size_t )
			{
				return ImplSlabAllocate(ObjectTypes::Process);
			}
			void operator delete(void* ptr)
			{
				ImplSlabFree(ObjectTypes::Process, ptr);
			}
			void* operator new[](size_t sz)
			{
				return ImplSlabAllocate(ObjectTypes::Process, sz / sizeof(Process));
			}
			void operator delete[](void* ptr, size_t sz)
			{
				ImplSlabFree(ObjectTypes::Process, ptr, sz / sizeof(Process));
			}
			[[nodiscard]] void* operator new(size_t, void* ptr) noexcept { return ptr; }
			[[nodiscard]] void* operator new[](size_t, void* ptr) noexcept { return ptr; }
			void operator delete(void*, void*) noexcept {}
			void operator delete[](void*, void*) noexcept {}
		};
		extern Process::ProcessList g_processes;
		Process* CreateProcess(bool isUsermode);
		// If process is the current thread's process, this function will not return.
		bool TerminateProcess(Process* process, bool isCurrentProcess = false);
		bool GracefullyTerminateProcess(Process* process);
	}
}