/*
	oboskrnl/multitasking/x86_64/setupFrameInfo.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>

#include <x86_64-utils/asm.h>

#include <arch/interrupt.h>
#include <arch/x86_64/memory_manager/virtual/allocate.h>
#include <arch/x86_64/memory_manager/virtual/initialize.h>

#include <multitasking/arch.h>
#include <multitasking/thread.h>

#define BIT(n) (1<<n)

namespace obos
{
	extern void kmain_common();
	namespace thread
	{
		void setupThreadContext(taskSwitchInfo* info, void* _stackInfo, uintptr_t entry, uintptr_t userdata, size_t stackSize, bool isUsermodeProgram)
		{
			if (stackSize == 0)
				stackSize = 0x8000;

			stackSize = ((stackSize + 0xfff) & (~0xfff));

			info->frame.cs = isUsermodeProgram ? 0x1b : 0x08;
			info->frame.ds = isUsermodeProgram ? 0x23 : 0x10;
			info->frame.ss = isUsermodeProgram ? 0x23 : 0x10;

			info->frame.rip = entry;
			info->frame.rdi = userdata;
			info->frame.rbp = 0;
			if ((void(*)())entry != kmain_common)
				info->frame.rsp = ((uintptr_t)memory::VirtualAlloc(nullptr, stackSize / 4096, static_cast<uintptr_t>(isUsermodeProgram) * memory::PROT_USER_MODE_ACCESS | memory::PROT_NO_COW_ON_ALLOCATE)) + (stackSize - 8);
			else
				info->frame.rsp = ((uintptr_t)memory::VirtualAlloc((void*)0xFFFFFFFF90000000, stackSize / 4096, memory::PROT_NO_COW_ON_ALLOCATE)) + (stackSize - 8);
			*(uintptr_t*)info->frame.rsp = 0;
			info->frame.rflags.setBit(x86_64_flags::RFLAGS_INTERRUPT_ENABLE | x86_64_flags::RFLAGS_CPUID);
			asm volatile("fninit; fxsave (%0)" : : "r"(info->fpuState) : "memory");

			Thread::StackInfo* stackInfo = (Thread::StackInfo*)_stackInfo;

			stackInfo->size = stackSize;
			stackInfo->addr = reinterpret_cast<void*>(info->frame.rsp - (stackSize - 8));

			if(isUsermodeProgram)
				info->frame.rflags.setBit(BIT(12) | BIT(13));


			info->cr3 = memory::getCurrentPageMap();
			if(isUsermodeProgram)
				info->tssStackBottom = memory::VirtualAlloc(nullptr, 4, memory::PROT_USER_MODE_ACCESS | memory::PROT_NO_COW_ON_ALLOCATE);
		}
		void freeThreadStackInfo(void* _stackInfo)
		{
			Thread::StackInfo* stackInfo = (Thread::StackInfo*)_stackInfo;
			memory::VirtualFree(stackInfo->addr, stackInfo->size / 4096);
			stackInfo->addr = nullptr;
			stackInfo->size = 0;
		}
	}
}