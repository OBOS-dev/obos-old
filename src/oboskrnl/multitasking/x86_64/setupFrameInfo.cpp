/*
	oboskrnl/multitasking/x86_64/setupFrameInfo.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <memory_manipulation.h>

#include <x86_64-utils/asm.h>

#include <arch/interrupt.h>
#include <allocators/vmm/vmm.h>
#include <arch/x86_64/memory_manager/virtual/initialize.h>

#include <multitasking/arch.h>
#include <multitasking/thread.h>

#include <multitasking/process/process.h>

#include <limine.h>

#include <utils/vector.h>

#define BIT(n) (1<<n)

namespace obos
{
	extern void kmain_common(byte* initrdDriverData, size_t initrdDriverSize);
	extern volatile limine_module_request module_request;
	extern volatile limine_kernel_file_request kernel_file;
	namespace thread
	{
		void setupThreadContext(taskSwitchInfo* info, void* _stackInfo, uintptr_t entry, uintptr_t userdata, size_t stackSize, memory::VirtualAllocator* vallocator, void* asProc)
		{
			if (stackSize == 0)
				stackSize = 0x8000;

			stackSize = ((stackSize + 0xfff) & (~0xfff));

			process::Process* _proc = (process::Process*)asProc;

			bool isUsermodeProgram = _proc ? _proc->isUsermode : false;

			info->frame.cs = isUsermodeProgram ? 0x23 : 0x08;
			info->frame.ss = isUsermodeProgram ? 0x1b : 0x10;
			info->frame.ds = info->frame.ss;

			info->frame.rip = entry;
			info->frame.rdi = userdata;
			info->frame.rbp = 0;
			info->fsbase = 0;
			info->gsbase = isUsermodeProgram ? 0 : rdmsr(0xC0000101 /* GS.Base */);
			if (entry != (uintptr_t)kmain_common)
			{
				uintptr_t stackProtFlags = memory::PROT_NO_COW_ON_ALLOCATE;
				if (isUsermodeProgram)
					stackProtFlags |= memory::PROT_USER_MODE_ACCESS;
				info->frame.rsp = ((uintptr_t)vallocator->VirtualAlloc(nullptr, stackSize, stackProtFlags)) + (stackSize - 8);
			}
			else
			{
				info->frame.rsp = ((uintptr_t)vallocator->VirtualAlloc((void*)0xFFFFFFFF90000000, stackSize, memory::PROT_NO_COW_ON_ALLOCATE)) + (stackSize - 8);
				// Find the initrd driver.
				byte* procExecutable = nullptr;
				size_t procExecutableSize = 0;
				for (size_t i = 0; i < module_request.response->module_count; i++)
				{
					if (utils::strcmp(module_request.response->modules[i]->path, "/obos/initrdDriver"))
					{
						procExecutable = (byte*)module_request.response->modules[i]->address;
						procExecutableSize = module_request.response->modules[i]->size;
						break;
					}
				}
				info->frame.rdi = (uintptr_t)utils::memcpy(new byte[procExecutableSize + 1], procExecutable, procExecutableSize);
				info->frame.rsi = (uintptr_t)procExecutableSize;
			}
			info->frame.rflags.setBit(x86_64_flags::RFLAGS_INTERRUPT_ENABLE | x86_64_flags::RFLAGS_CPUID);
			asm volatile("fninit; fxsave (%0)" : : "r"(info->fpuState) : "memory");

			Thread::StackInfo* stackInfo = (Thread::StackInfo*)_stackInfo;

			stackInfo->size = stackSize;
			stackInfo->addr = reinterpret_cast<void*>(info->frame.rsp - (stackSize - 8));

			if(isUsermodeProgram)
				info->frame.rflags.setBit(x86_64_flags::RFLAGS_IOPL_3);

			info->cr3 = _proc ? _proc->context.cr3 : memory::getCurrentPageMap();
			info->tssStackBottom = vallocator->VirtualAlloc(nullptr, 0x4000, memory::PROT_NO_COW_ON_ALLOCATE);
			if (isUsermodeProgram)
				info->syscallStackBottom = vallocator->VirtualAlloc(nullptr, 0x4000, memory::PROT_NO_COW_ON_ALLOCATE); // Do not set user mode, or you'll page fault (SMAP).
		}
		void freeThreadStackInfo(void* _stackInfo, memory::VirtualAllocator* vallocator)
		{
			Thread::StackInfo* stackInfo = (Thread::StackInfo*)_stackInfo;
			vallocator->VirtualFree(stackInfo->addr, stackInfo->size);
			stackInfo->addr = nullptr;
			stackInfo->size = 0;
		}
	}
}