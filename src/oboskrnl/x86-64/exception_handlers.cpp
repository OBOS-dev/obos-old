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
		if (multitasking::g_initialized)
		{
			if (multitasking::g_currentThread->owner && multitasking::g_currentThread->owner->isUserMode)
				multitasking::g_currentThread->owner->TerminateProcess(0xFFFFFFF1);
			if(multitasking::g_currentThread->owner)
				pid = multitasking::g_currentThread->owner->pid;
		}
		if (utils::testBitInBitfield(frame->errorCode, 4))
			action = "execute";
		if (!inRange(location, s_backbuffer, s_backbuffer + 1024 * 768))
			kpanic((PVOID)frame->rbp, (PVOID)frame->rip,
				kpanic_format(
					"Page fault in %s-mode at %p (tid %d, pid %d) while trying to %s a %s page.\r\nThe address of that page is %p. Error code: %d\r\nDumping registers: \r\n"
					"\tRDI: %p\r\n"
					"\tRSI: %p\r\n"
					"\tRBP: %p\r\n"
					"\tRSP: %p\r\n"
					"\tRBX: %p\r\n"
					"\tRDX: %p\r\n"
					"\tRCX: %p\r\n"
					"\tRAX: %p\r\n"
					"\tRIP: %p\r\n"
					"\tR8: %p\r\n"
					"\tR9: %p\r\n"
					"\tR10: %p\r\n"
					"\tR11: %p\r\n"
					"\tR12: %p\r\n"
					"\tR13: %p\r\n"
					"\tR14: %p\r\n"
					"\tR15: %p\r\n"
					"\tRFLAGS: %p\r\n"
					"\tSS: %p\r\n"
					"\tDS: %p\r\n"
					"\tCS: %p\r\n"),
				privilegeLevel, frame->rip, multitasking::GetCurrentThreadTid(), pid, action, isPresent, location,
				frame->errorCode, // Some bits are not translated by the handler.
				frame->rdi, frame->rsi, frame->rbp, frame->rsp, frame->rbx,
				frame->rdx, frame->rcx, frame->rax, frame->rip, 
				frame->r8, frame->r9, frame->r10, frame->r11,
				frame->r12, frame->r13, frame->r14, frame->r15,
				frame->rflags,
				frame->ss, frame->ds, frame->cs);
		asm volatile("cli; hlt");
	}
	void defaultExceptionHandler(const interrupt_frame* frame)
	{
		if (frame->errorCode == 1 && utils::testBitInBitfield(frame->rflags, 8))
		{
			utils::clearBitInBitfield(const_cast<interrupt_frame*>(frame)->rflags, 8);
			return;
		}
		DWORD pid = 0xFFFFFFFF;
		if (multitasking::g_initialized)
		{
			if (multitasking::g_currentThread->owner && multitasking::g_currentThread->owner->isUserMode)
				multitasking::g_currentThread->owner->TerminateProcess(~frame->intNumber);
			if(multitasking::g_currentThread->owner)
				pid = multitasking::g_currentThread->owner->pid;
		}
		kpanic((PVOID)frame->rbp, (PVOID)frame->rip,
			kpanic_format(
				"Unhandled exception %d at %p (tid %d, pid %d). Error code: %d. Dumping registers: \r\n"
				"\tRDI: %p\r\n"
				"\tRSI: %p\r\n"
				"\tRBP: %p\r\n"
				"\tRSP: %p\r\n"
				"\tRBX: %p\r\n"
				"\tRDX: %p\r\n"
				"\tRCX: %p\r\n"
				"\tRAX: %p\r\n"
				"\tRIP: %p\r\n"
				"\tR8: %p\r\n"
				"\tR9: %p\r\n"
				"\tR10: %p\r\n"
				"\tR11: %p\r\n"
				"\tR12: %p\r\n"
				"\tR13: %p\r\n"
				"\tR14: %p\r\n"
				"\tR15: %p\r\n"
				"\tRFLAGS: %p\r\n"
				"\tSS: %p\r\n"
				"\tDS: %p\r\n"
				"\tCS: %p\r\n"),
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