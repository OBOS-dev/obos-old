/*
	oboskrnl/process/x86_64/procInfo.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>

#include <multitasking/process/arch.h>

#include <multitasking/cpu_local.h>
#include <multitasking/arch.h>

#include <multitasking/threadAPI/thrHandle.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>
#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <memory_manipulation.h>

#include <x86_64-utils/asm.h>

#include <vfs/devManip/driveHandle.h>
#include <vfs/fileManip/fileHandle.h>

namespace obos
{
	extern void SetIST(void* rsp);
	namespace memory
	{
		extern uintptr_t s_pageDirectoryPhys;
	}
	namespace process
	{
		void setupContextInfo(procContextInfo* info)
		{
			uintptr_t flags = saveFlagsAndCLI();
			info->cr3 = (void*)memory::allocatePhysicalPage();
			uintptr_t* newPageMap = memory::mapPageTable((uintptr_t*)info->cr3);
			utils::memzero(newPageMap, 4096);
			uintptr_t* pageMap = memory::mapPageTable((uintptr_t*)memory::getCurrentPageMap());
			for (size_t pml4I = memory::PageMap::addressToIndex(0xffff'8000'0000'0000, 3); pml4I < 512; pml4I++)
				newPageMap[pml4I] = pageMap[pml4I];
			restorePreviousInterruptStatus(flags);
		}
		void freeProcessContext(procContextInfo* info)
		{
			//memory::freePhysicalPage((uintptr_t)info->cr3);
			for (auto iter = info->handleTable.begin(); iter; iter++)
			{
				auto &hnd_val = *(*iter).value;
				switch (hnd_val.second)
				{
				case obos::syscalls::ProcessHandleType::FILE_HANDLE:
					delete (vfs::FileHandle*)hnd_val.first;
					break;
				case obos::syscalls::ProcessHandleType::DRIVE_HANDLE:
					delete (vfs::DriveHandle*)hnd_val.first;
					break;
				case obos::syscalls::ProcessHandleType::THREAD_HANDLE:
					delete (thread::ThreadHandle*)hnd_val.first;
					break;
				default:
					break;
				}
			}
			info->handleTable.erase();
		}
		void switchToProcessContext(procContextInfo* info)
		{
			memory::PageMap* cr3 = (memory::PageMap*)info->cr3;
			thread::cpu_local* _info = thread::GetCurrentCpuLocalPtr();
			SetIST((byte*)_info->temp_stack.addr + _info->temp_stack.size);
			cr3->switchToThis(); // Warning: May page fault because of the stack being unmapped in the other process' context.
		}

		void putThreadAtFunctionWithCPUTempStack(thread::Thread *thread, void *function, void *par1, void *par2)
		{
			auto cpu_local = thread::GetCurrentCpuLocalPtr();
			uintptr_t rsp = (uintptr_t)cpu_local->temp_stack.addr + cpu_local->temp_stack.size;
			thread->context.frame.rip = (uintptr_t)function;
			thread->context.frame.rdi = (uintptr_t)par1;
			thread->context.frame.rsi = (uintptr_t)par2;
			thread->context.frame.rsp =            rsp;
			thread->context.frame.cs = 0x08;
			thread->context.frame.ss = 0x10;
			thread->context.frame.ds = 0x10;
			thread->context.frame.rflags = getEflags();
			thread::switchToThreadImpl(&thread->context, thread);
			// Should never get hit, as switchToThreadImpl on x86_64 is [[noreturn]].
			while(1);
		}
    }
}