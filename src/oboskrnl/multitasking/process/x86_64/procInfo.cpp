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
			memory::PageMap* currentPageMap = memory::getCurrentPageMap();
			uintptr_t entry80000000 = (uintptr_t)currentPageMap->getL4PageMapEntryAt(0xffffffff80000000);
			uintptr_t entryffff80000000_0 = (uintptr_t)currentPageMap->getL4PageMapEntryAt(0xffff800000000000);
			memory::PageMap* newPageMap = (memory::PageMap*)memory::allocatePhysicalPage();
			uintptr_t* pPageMap = (uintptr_t*)newPageMap;
			utils::memzero(memory::mapPageTable(pPageMap), 4096);
			memory::mapPageTable(pPageMap)[511] = entry80000000;
			memory::mapPageTable(pPageMap)[memory::PageMap::addressToIndex(0xffff800000000000, 3)] = entryffff80000000_0;
			memory::mapPageTable(reinterpret_cast<uintptr_t*>((uintptr_t)newPageMap->getL4PageMapEntryAt(0xfffffffffffff000) & 0xFFFFFFFFFF000))
				[memory::PageMap::addressToIndex(0xfffffffffffff000, 2)] = memory::s_pageDirectoryPhys | 3;
			info->cr3 = newPageMap;
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
					// TODO: Implement freeing file/drive handles.
				case obos::syscalls::ProcessHandleType::FILE_HANDLE:
					break;
				case obos::syscalls::ProcessHandleType::DRIVE_HANDLE:
					break;
				case obos::syscalls::ProcessHandleType::THREAD_HANDLE:
					delete ((thread::ThreadHandle*)hnd_val.first);
					break;
				default:
					break;
				}
			}
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
			thread::switchToThreadImpl(&thread->context, thread);
			// Should never get hit, as switchToThreadImpl on x86_64 is [[noreturn]].
			while(1);
		}
    }
}