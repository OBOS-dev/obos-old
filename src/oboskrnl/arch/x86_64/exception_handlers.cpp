/*
	oboskrnl/arch/x86_64/exception_handlers.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>

#include <arch/interrupt.h>

#include <x86_64-utils/asm.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>
#include <arch/x86_64/memory_manager/virtual/allocate.h>

#include <arch/x86_64/memory_manager/physical/allocate.h>

namespace obos
{
	void exception14(interrupt_frame* frame)
	{
		uintptr_t entry = 0;
		if (frame->errorCode & 1)
		{
			uintptr_t faultAddress = (uintptr_t)getCR2() & ~0xfff;
			memory::PageMap* pageMap = memory::getCurrentPageMap();
			entry = (uintptr_t)pageMap->getL1PageMapEntryAt(faultAddress);
			if (entry & ((uintptr_t)1 << 9))
			{
				uintptr_t flags = memory::DecodeProtectionFlags(entry >> 52) | 1;
				uintptr_t newEntry = memory::allocatePhysicalPage();
				utils::memcpy(memory::mapPageTable((uintptr_t*)newEntry), (void*)faultAddress, 4096);
				newEntry |= flags;
				memory::MapVirtualPageToEntry((void*)faultAddress, newEntry);
				logger::log("Page fault at %p. Old PTE: %p. New PTE: %p.\n", faultAddress, entry, newEntry);
				return;
			}
		}
		logger::panic("Page fault in %s-mode at %p while trying to %s a %s page. The address of this page is %p. Error code: %d. PTE: %p. Dumping registers:\n"
					  "\tRDI: %p, RSI: %p, RBP: %p\n"
					  "\tRSP: %p, RBX: %p, RDX: %p\n"
					  "\tRCX: %p, RAX: %p, RIP: %p\n"
					  "\t R8: %p,  R9: %p, R10: %p\n"
					  "\tR11: %p, R12: %p, R13: %p\n"
					  "\tR14: %p, R15: %p, RFL: %p\n"
					  "\t SS: %p,  DS: %p,  CS: %p\n",
			(frame->errorCode & ((uintptr_t)1 << 2)) ? "user" : "kernel",
			frame->rip,
			(frame->errorCode & ((uintptr_t)1 << 1)) ? "write" : "read",
			(frame->errorCode & ((uintptr_t)1 << 0)) ? "present" : "non-present",
			getCR2(),
			frame->errorCode, 
			entry,
			frame->rdi, frame->rsi, frame->rbp,
			frame->rsp, frame->rbx, frame->rdx,
			frame->rcx, frame->rax, frame->rip,
			frame-> r8, frame-> r9, frame->r10,
			frame->r11, frame->r12, frame->r13,
			frame->r14, frame->r15, frame->rflags,
			frame-> ss, frame-> ds, frame-> cs
		);
	}
	void RegisterExceptionHandlers()
	{
		RegisterInterruptHandler(14, exception14);
	}
}