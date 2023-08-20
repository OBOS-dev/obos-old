/*
	process.h

	Copyright (c) 2023 oboskrnl/process/process.h
*/

#include <process/process.h>

#include <memory_manager/paging/allocate.h>
#include <memory_manager/paging/init.h>

#include <multitasking/multitasking.h>
#include <multitasking/threadHandle.h>
#include <multitasking/thread.h>

#include <elf/elf.h>

#include <inline-asm.h>
#include <handle.h>
#include <types.h>

#include <utils/memory.h>
#include <utils/list.h>


extern "C" UINTPTR_T boot_page_directory1;

namespace obos
{
	extern process::Process* g_kernelProcess;
	namespace memory
	{
		extern UINTPTR_T* kmap_pageTable(PVOID physicalAddress);
	}
	namespace process
	{
		DWORD g_nextProcessId = 0;
		list_t* g_processList = nullptr;

		Process::Process()
		{}

		VOID attribute(section(".glb.text")) ProcEntryPoint(PVOID entry)
		{
			DWORD(*main)() = (DWORD(*)())entry;
			main();
			asm volatile("push %eax; mov $1, %eax; mov %esp, %ebx; int $0x40;");
		}
		VOID DriverEntryPoint(PVOID entry)
		{
			DWORD(*main)() = (DWORD(*)())entry;
			main();
			EnterKernelSection();
			auto currentThread = multitasking::GetCurrentThreadHandle();
			currentThread->SetThreadPriority(multitasking::Thread::priority_t::IDLE);
			currentThread->closeHandle();
			delete currentThread;
			multitasking::g_currentThread->isBlockedCallback = [](multitasking::Thread*, PVOID)->bool { return false; };
			multitasking::g_currentThread->status |= (DWORD)multitasking::Thread::status_t::BLOCKED;
			LeaveKernelSection();
			_int(0x30);
		}

		DWORD Process::CreateProcess(PBYTE file, SIZE_T size, PVOID _mainThread, bool isDriver)
		{
			if (pageDirectory)
				return (DWORD)-1;
			if (!g_processList)
				g_processList = list_new();
			EnterKernelSection();
			pid = g_nextProcessId++;
			pageDirectory = new memory::PageDirectory{};
			UINTPTR_T* newPageDirectory = memory::kmap_pageTable(pageDirectory->getPageDirectory());
			// We need to make sure the page directory is empty.
			// Otherwise, you'll have a bad time.
			utils::memzero(newPageDirectory, 4096);
			UINTPTR_T* pBoot_page_directory1 = &boot_page_directory1;
			// Clone the kernel page directory.
			for (int i = 0; i < 1024; i++)
				if(pBoot_page_directory1[i])
					newPageDirectory[i] = pBoot_page_directory1[i];
			// Leap of faith!
			pageDirectory->switchToThis();
			
			children = list_new();
			threads = list_new();
			abstractHandles = list_new();

			UINTPTR_T entry = 0;
			UINTPTR_T base = 0;

			DWORD loaderRet = elfLoader::LoadElfFile(file, size, entry, base);
			if (loaderRet)
			{
				multitasking::g_currentThread->owner->pageDirectory->switchToThis();
				LeaveKernelSection();
				return loaderRet;
			}

			isUserMode = !isDriver;

			multitasking::Thread* mainThread = new multitasking::Thread{};
			
			mainThread->owner = this;

			DWORD ret = mainThread->CreateThread(multitasking::Thread::priority_t::NORMAL,
				!isDriver ? (VOID(*)(PVOID))0x400000 : DriverEntryPoint,
				(PVOID)entry,
				(DWORD)multitasking::Thread::status_t::PAUSED,
				0);
			if (ret)
			{
				multitasking::g_currentThread->owner->pageDirectory->switchToThis();
				LeaveKernelSection();
				return 3 + ret;
			}

			mainThread->owner = this;
			parent = multitasking::g_currentThread->owner;

			list_rpush(threads, list_node_new(mainThread));
			list_rpush(multitasking::g_currentThread->owner->children, list_node_new(this));

			if (_mainThread)
			{
				multitasking::ThreadHandle* mainThreadHandle = (multitasking::ThreadHandle*)_mainThread;
				mainThreadHandle->OpenThread(mainThread);
			}
			
			// Switch back the parent's page directory.
			multitasking::g_currentThread->owner->pageDirectory->switchToThis();

			list_rpush(g_processList, list_node_new(this));

			LeaveKernelSection();

			mainThread->status &= ~((DWORD)multitasking::Thread::status_t::PAUSED);

			return 0;
		}
		static BYTE s_temporaryStack[8192] attribute(aligned(4096));
		static PBYTE s_localVariable = nullptr;
		static DWORD s_exitCode = 0;
		DWORD Process::TerminateProcess(DWORD exitCode)
		{
			if (!pageDirectory)
				return (DWORD)-1;
			EnterKernelSection();
			pageDirectory->switchToThis();
			// Iterate over the children.
			list_iterator_t* iter = list_iterator_new(children, LIST_HEAD);
			for (list_node_t* _child = list_iterator_next(iter); _child != nullptr; _child = list_iterator_next(iter))
			{
				Process* child = (Process*)_child->val;
				// The kernel is very nice and is adopting the children.
				child->parent = g_kernelProcess;
			}
			list_iterator_destroy(iter);
			// As for the threads, they die.
			iter = list_iterator_new(threads, LIST_HEAD);
			for (list_node_t* _thread = list_iterator_next(iter); _thread != nullptr; _thread = list_iterator_next(iter))
			{
				multitasking::ThreadHandle* threadHandle = new multitasking::ThreadHandle{};
				threadHandle->OpenThread((multitasking::Thread*)_thread->val);
				threadHandle->TerminateThread(0);
				threadHandle->closeHandle();
				delete threadHandle;
			}
			list_iterator_destroy(iter);
			iter = list_iterator_new(abstractHandles, LIST_HEAD);
			// Same for the rest of the handles.
			for (list_node_t* _handle = list_iterator_next(iter); _handle != nullptr; _handle = list_iterator_next(iter))
			{
				Handle* handle = (Handle*)_handle->val;
				handle->closeHandle();
			}
			list_iterator_destroy(iter);
			list_destroy(children);
			list_destroy(threads);
			list_destroy(abstractHandles);
			list_remove(g_processList, list_find(g_processList, this));
			s_exitCode = exitCode;
			if (this == multitasking::g_currentThread->owner)
			{
				UINTPTR_T* stack = (UINTPTR_T*)s_temporaryStack;
				utils::memzero(s_temporaryStack, 8192);
				stack[2047] = (UINTPTR_T)this;
				// If we free all the memory for the process, that would include the current thread's stack, thus causing a triple fault.
				// The only thing that could go wrong is if there is a cpu exception that enables interrupts when it returns, then the scheduler gets called, switching to another
				// thread that also exits the process, then it will be overrwritten, and when this thread resumes execution, it would've been accessing garbage values, and would probably
				// page fault.
				stack += 2045;
				asm volatile("mov %0, %%ebp;"
							 "mov %0, %%esp;" : : "r"(stack) : "memory");
			}
			// Free everything.
			for (s_localVariable = (PBYTE)0x400000; s_localVariable != (PBYTE)0xC0000000; s_localVariable += 4096)
				memory::VirtualFree(s_localVariable, 1);
			for (s_localVariable = (PBYTE)0xE0000000; s_localVariable != (PBYTE)0xFFC00000; s_localVariable += 4096)
				memory::VirtualFree(s_localVariable, 1);
			delete pageDirectory;
			this->exitCode = s_exitCode;
			if (this == multitasking::g_currentThread->owner)
			{
				LeaveKernelSection();
				multitasking::ExitThread(0);
			}
			LeaveKernelSection();
			return 0;
		}
	}
}