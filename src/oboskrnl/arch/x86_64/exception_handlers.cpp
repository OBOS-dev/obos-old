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
		memory::PageMap* pageMap = memory::getCurrentPageMap();
		uintptr_t faultAddress = (uintptr_t)getCR2() & ~0xfff;
		if (frame->errorCode & 1)
		{
			entry = (uintptr_t)pageMap->getL1PageMapEntryAt(faultAddress);
			if (entry & ((uintptr_t)1 << 9))
			{
				uintptr_t flags = memory::DecodeProtectionFlags(entry >> 52) | 1;
				uintptr_t newEntry = memory::allocatePhysicalPage();
				utils::memcpy(memory::mapPageTable((uintptr_t*)newEntry), (void*)faultAddress, 4096);
				newEntry |= flags;
				memory::MapVirtualPageToEntry((void*)faultAddress, newEntry);
				return;
			}
		}
		const char* action = (frame->errorCode & ((uintptr_t)1 << 1)) ? "write" : "read";
		if (frame->errorCode & ((uintptr_t)1 << 4))
			action = "execute";
		logger::panic("Page fault in %s-mode at %p while trying to %s a %s page. The address of this page is %p. Error code: %d.\nPTE: %p, PDE: %p, PDPE: %p, PME: %p.\nDumping registers:\n"
			"\tRDI: %p, RSI: %p, RBP: %p\n"
			"\tRSP: %p, RBX: %p, RDX: %p\n"
			"\tRCX: %p, RAX: %p, RIP: %p\n"
			"\t R8: %p,  R9: %p, R10: %p\n"
			"\tR11: %p, R12: %p, R13: %p\n"
			"\tR14: %p, R15: %p, RFL: %p\n"
			"\t SS: %p,  DS: %p,  CS: %p\n",
			(frame->errorCode & ((uintptr_t)1 << 2)) ? "user" : "kernel",
			frame->rip,
			action,
			(frame->errorCode & ((uintptr_t)1 << 0)) ? "present" : "non-present",
			getCR2(),
			frame->errorCode,
			entry,
			pageMap->getL2PageMapEntryAt(faultAddress),
			pageMap->getL3PageMapEntryAt(faultAddress),
			pageMap->getL4PageMapEntryAt(faultAddress),
			frame->rdi, frame->rsi, frame->rbp,
			frame->rsp, frame->rbx, frame->rdx,
			frame->rcx, frame->rax, frame->rip,
			frame-> r8, frame-> r9, frame->r10,
			frame->r11, frame->r12, frame->r13,
			frame->r14, frame->r15, frame->rflags,
			frame-> ss, frame-> ds, frame-> cs
		);
	}
	void defaultExceptionHandler(interrupt_frame* frame)
	{
		logger::panic("Exception %d at %p. Error code: %d.\nDumping registers:\n"
			"\tRDI: %p, RSI: %p, RBP: %p\n"
			"\tRSP: %p, RBX: %p, RDX: %p\n"
			"\tRCX: %p, RAX: %p, RIP: %p\n"
			"\t R8: %p,  R9: %p, R10: %p\n"
			"\tR11: %p, R12: %p, R13: %p\n"
			"\tR14: %p, R15: %p, RFL: %p\n"
			"\t SS: %p,  DS: %p,  CS: %p\n",
			frame->intNumber,
			frame->rip,
			frame->errorCode,
			frame->rdi, frame->rsi, frame->rbp,
			frame->rsp, frame->rbx, frame->rdx,
			frame->rcx, frame->rax, frame->rip,
			frame->r8, frame->r9, frame->r10,
			frame->r11, frame->r12, frame->r13,
			frame->r14, frame->r15, frame->rflags,
			frame->ss, frame->ds, frame->cs
		);
	}
	void RegisterExceptionHandlers()
	{
		for (byte i = 0; i < 14; i++)
			RegisterInterruptHandler(i, defaultExceptionHandler);
		RegisterInterruptHandler(14, exception14);
		for (byte i = 15; i < 32; i++)
			RegisterInterruptHandler(i, defaultExceptionHandler);
	}
}