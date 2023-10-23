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

#define BIT(n) (1<<n)

namespace obos
{
	namespace thread
	{
		void setupThreadContext(taskSwitchInfo* info, uintptr_t entry, uintptr_t userdata, size_t stackSize, bool isUsermodeProgram)
		{
			if (stackSize == 0)
				stackSize = 8192;

			stackSize = ((stackSize + 0xfff) & (~0xfff));

			info->frame.cs = isUsermodeProgram ? 0x1b : 0x08;
			info->frame.ds = isUsermodeProgram ? 0x23 : 0x10;
			info->frame.ss = isUsermodeProgram ? 0x23 : 0x10;

			info->frame.rip = entry;
			info->frame.rdi = userdata;
			info->frame.rbp = 0;
			info->frame.rsp = ((uintptr_t)memory::VirtualAlloc(nullptr, stackSize >> 12, static_cast<uintptr_t>(isUsermodeProgram) * memory::PROT_USER_MODE_ACCESS | memory::PROT_NO_COW_ON_ALLOCATE)) + (stackSize - 8);
			*(uintptr_t*)info->frame.rsp = 0;
			info->frame.rflags.setBit(x86_64_flags::RFLAGS_INTERRUPT_ENABLE | x86_64_flags::RFLAGS_CPUID);

			if(isUsermodeProgram)
				info->frame.rflags.setBit(BIT(12) | BIT(13));


			info->cr3 = memory::getCurrentPageMap();
			if(isUsermodeProgram)
				info->tssStackBottom = memory::VirtualAlloc(nullptr, 4, memory::PROT_USER_MODE_ACCESS | memory::PROT_NO_COW_ON_ALLOCATE);
		}
	}
}