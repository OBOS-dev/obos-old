/*
	oboskrnl/x86-64/exception_handlers.cpp

	Copyright (c) 2023 Omar Berrow	
*/

#include <descriptors/idt/x86-64/idt.h>

#include <klog.h>

#include <memory_manager/paging/init.h>

#include <memory_manager/physical.h>

#include <utils/bitfields.h>
#include <utils/memory.h>

#include <types.h>

#include <multitasking/threadHandle.h>
#include <multitasking/multitasking.h>

#define inRange(val, rStart, rEnd) (((UINTPTR_T)(val)) >= ((UINTPTR_T)(rStart)) && ((UINTPTR_T)(val)) < ((UINTPTR_T)(rEnd)))

#define getDebugRegister(dest, src) asm volatile ("mov %% " #src ", %0":"=r"(dest) : : "memory")

namespace obos
{
	namespace memory
	{
		extern UINTPTR_T* kmap_pageTable(PVOID physicalAddress);
		extern bool CPUSupportsExecuteDisable();
	}
	void pageFault(const interrupt_frame* frame)
	{
		const char* action = utils::testBitInBitfield(frame->errorCode, 1) ? "write" : "read";
		const char* privilegeLevel = utils::testBitInBitfield(frame->errorCode, 2) ? "user" : "kernel";
		const char* isPresent = utils::testBitInBitfield(frame->errorCode, 0) ? "present" : "non-present";
		UINTPTR_T location = (UINTPTR_T)memory::GetPageFaultAddress();
		extern UINT32_T* s_backbuffer;
		DWORD pid = 0xFFFFFFFF;
		if (utils::testBitInBitfield(frame->errorCode, 0))
		{
			// The page was present...
			if (utils::testBitInBitfield(memory::g_level4PageMap->getRealPageTableEntry(
					memory::PageMap::computeIndexAtAddress(location & (~0xfff), 3),
					memory::PageMap::computeIndexAtAddress(location & (~0xfff), 2),
					memory::PageMap::computeIndexAtAddress(location & (~0xfff), 1),
					memory::PageMap::computeIndexAtAddress(location & (~0xfff), 0)), 9))
			{
				UINTPTR_T* pageTablePhys = nullptr;
				// Allocate the page, and apply the flags.
				UINTPTR_T* pageTable = 
					memory::g_level4PageMap->getPageTableRealtive(
							memory::PageMap::computeIndexAtAddress(location & ~(0xfff), 3), 
							memory::PageMap::computeIndexAtAddress(location & ~(0xfff), 2),
							memory::PageMap::computeIndexAtAddress(location & ~(0xfff), 1));
				pageTablePhys = pageTable;
				pageTablePhys = reinterpret_cast<UINTPTR_T*>(*pageTablePhys & 0xFFFFFFFFFF000);
				pageTable = memory::kmap_pageTable(pageTablePhys);
				UINTPTR_T* entry = &pageTable[memory::PageMap::computeIndexAtAddress(location, 0)];
				UINTPTR_T flags = (*entry >> 52) & 0b11111;
				//utils::clearBitInBitfield(flags, 6);
				bool canExecute = !utils::testBitInBitfield(flags, 8);
				UINTPTR_T newEntry = reinterpret_cast<UINTPTR_T>(memory::kalloc_physicalPage());
				newEntry |= 1 | flags;
				if(!canExecute && memory::CPUSupportsExecuteDisable())
					utils::setBitInBitfield(newEntry, 63);
				pageTable = memory::kmap_pageTable(pageTablePhys);
				*entry = newEntry;
				memory::tlbFlush(location & (~0xFFF));
				// Zero out the page, as it would be weird if the program writes to the memory, then the value they wrote is in there, with a lot of garbage 
				// (or possibly sensitive data from another program) that wasn't there before.
				utils::dwMemset(reinterpret_cast<DWORD*>(memory::kmap_pageTable(PVOID(newEntry & 0xFFFFFFFFFF000))), 0, 1024);
				return; // Make it out like nothing happened...
			}
		}
		if (multitasking::g_initialized && multitasking::g_currentThread->owner)
		{
			/*if (multitasking::g_currentThread->owner->isUserMode)
				multitasking::g_currentThread->owner->TerminateProcess(0xFFFFFFF1);*/
			if (!multitasking::g_currentThread->owner->isUserMode && utils::testBitInBitfield(frame->errorCode, 2) && utils::testBitInBitfield(frame->errorCode, 0))
			{
				// A kernel mode process was put into ring 3 by accident, so we put them back to ring 0.
				interrupt_frame* _frame = const_cast<interrupt_frame*>(frame);
				_frame->cs = 0x08;
				_frame->ds = _frame->ss = 0x10;
				return;
			}
				
			pid = multitasking::g_currentThread->owner->pid;
		}
		if (utils::testBitInBitfield(frame->errorCode, 4))
			action = "execute";
		UINTPTR_T entry = 0;
		if (utils::testBitInBitfield(frame->errorCode, 0))
			entry = memory::g_level4PageMap->getRealPageTableEntry(
				memory::PageMap::computeIndexAtAddress(location, 3),
				memory::PageMap::computeIndexAtAddress(location, 2),
				memory::PageMap::computeIndexAtAddress(location, 1),
				memory::PageMap::computeIndexAtAddress(location, 0));
		if (!inRange(location, s_backbuffer, s_backbuffer + 1024 * 768))
			kpanic((PVOID)frame->rbp, (PVOID)frame->rip,
					"Page fault in %s-mode at %p (tid %d, pid %d) while trying to %s a %s page.\r\nThe address of that page is %p. Error code: %d. Page table entry: %p.\r\nDumping registers: \r\n"
				"\tRDI: %p, RSI: %p, RBP: %p\r\n"
				"\tRSP: %p, RBX: %p, RDX: %p\r\n"
				"\tRCX: %p, RAX: %p, RIP: %p\r\n"
				"\t R8: %p,  R9: %p, R10: %p\r\n"
				"\tR11: %p, R12: %p, R13: %p\r\n"
				"\tR14: %p, R15: %p, RFL: %p\r\n"
				"\t SS: %p,  DS: %p,  CS: %p\r\n",
				privilegeLevel, frame->rip, multitasking::GetCurrentThreadTid(), pid, action, isPresent, location,
				frame->errorCode, // Some bits are not translated by the handler.
				entry,
				frame->rdi, frame->rsi, frame->rbp, frame->rsp, frame->rbx,
				frame->rdx, frame->rcx, frame->rax, frame->rip, 
				frame->r8, frame->r9, frame->r10, frame->r11,
				frame->r12, frame->r13, frame->r14, frame->r15,
				frame->rflags,
				frame->ss, frame->ds, frame->cs);
		asm volatile("cli; hlt");
	}
	void debugExceptionHandler(const interrupt_frame* frame)
	{
		DWORD pid = 0xFFFFFFFF;
		if (multitasking::g_initialized)
		{
			if (multitasking::g_currentThread->owner)
				pid = multitasking::g_currentThread->owner->pid;
		}
		UINTPTR_T dr0 = 0, dr1 = 0, dr2 = 0, dr3 = 0, dr6 = 0, dr7 = 0;
		getDebugRegister(dr0,dr0);
		getDebugRegister(dr1,dr1);
		getDebugRegister(dr2,dr2);
		getDebugRegister(dr3,dr3);
		getDebugRegister(dr6,dr6);
		getDebugRegister(dr7,dr7);
		printf("Debug exception at %p (tid %d, pid %d). Error code: %d. Dumping registers: \r\n"
			"\tRDI: %p, RSI: %p, RBP: %p\r\n"
			"\tRSP: %p, RBX: %p, RDX: %p\r\n"
			"\tRCX: %p, RAX: %p, RIP: %p\r\n"
			"\t R8: %p,  R9: %p, R10: %p\r\n"
			"\tR11: %p, R12: %p, R13: %p\r\n"
			"\tR14: %p, R15: %p, RFL: %p\r\n"
			"\t SS: %p,  DS: %p,  CS: %p\r\n",
			"\tDR0: %p, \tDR1: %p, \tDR2: %p\r\n"
			"\tDR3: %p, \tDR6: %p, \tDR7: %p\r\n",
			frame->rip,
			multitasking::GetCurrentThreadTid(), pid,
			frame->errorCode,
			frame->rdi, frame->rsi, frame->rbp, frame->rsp, frame->rbx,
			frame->rdx, frame->rcx, frame->rax, frame->rip,
			frame->r8, frame->r9, frame->r10, frame->r11,
			frame->r12, frame->r13, frame->r14, frame->r15,
			frame->rflags,
			frame->ss, frame->ds, frame->cs,
			dr0, dr1, dr2, dr3, dr6, dr7);
		printf("Stack trace:\n");
		printStackTrace((PVOID)frame->rbp, "\t");
		disassemble((PVOID)frame->rip, "\t");
		return;
	}
	void defaultExceptionHandler(const interrupt_frame* frame)
	{
		DWORD pid = 0xFFFFFFFF;
		if (multitasking::g_initialized)
		{
			if (multitasking::g_currentThread->owner && multitasking::g_currentThread->owner->isUserMode)
				multitasking::g_currentThread->owner->TerminateProcess(~frame->intNumber);
			if(multitasking::g_currentThread->owner)
				pid = multitasking::g_currentThread->owner->pid;
		}
		kpanic((PVOID)frame->rbp, (PVOID)frame->rip,
				"Unhandled exception %d at %p (tid %d, pid %d). Error code: %d. Dumping registers: \r\n"
			"\tRDI: %p, RSI: %p, RBP: %p\r\n"
			"\tRSP: %p, RBX: %p, RDX: %p\r\n"
			"\tRCX: %p, RAX: %p, RIP: %p\r\n"
			"\t R8: %p,  R9: %p, R10: %p\r\n"
			"\tR11: %p, R12: %p, R13: %p\r\n"
			"\tR14: %p, R15: %p, RFL: %p\r\n"
			"\t SS: %p,  DS: %p,  CS: %p\r\n",
			frame->intNumber,
			frame->rip,
			multitasking::GetCurrentThreadTid(), pid,
			frame->errorCode,
			frame->rdi, frame->rsi, frame->rbp, frame->rsp, frame->rbx,
			frame->rdx, frame->rcx, frame->rax, frame->rip,
			frame->r8, frame->r9, frame->r10, frame->r11,
			frame->r12, frame->r13, frame->r14, frame->r15,
			frame->rflags,
			frame->ss, frame->ds, frame->cs);
	}
}