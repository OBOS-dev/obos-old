/*
	oboskrnl/i686/exception_handlers.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <descriptors/idt/idt.h>

namespace obos
{
	void pageFault(const interrupt_frame* frame)
	{
		const char* action = utils::testBitInBitfield(frame->errorCode, 1) ? "write" : "read";
		const char* privilegeLevel = utils::testBitInBitfield(frame->errorCode, 2) ? "user" : "kernel";
		const char* isPresent = utils::testBitInBitfield(frame->errorCode, 0) ? "present" : "non-present";
		UINTPTR_T location = (UINTPTR_T)memory::GetPageFaultAddress();
		extern UINT32_T* s_backbuffer;
		DWORD pid = 0xFFFFFFFF;
		if (multitasking::g_initialized)
		{
			if (multitasking::g_currentThread->owner && multitasking::g_currentThread->owner->isUserMode)
				multitasking::g_currentThread->owner->TerminateProcess(0xFFFFFFF1);
			pid = multitasking::g_currentThread->owner->pid;
		}
		if (!inRange(location, s_backbuffer, s_backbuffer + 1024 * 768))
			kpanic((PVOID)frame->ebp, (PVOID)frame->eip,
				kpanic_format(
					"Page fault in %s-mode at %p (tid %d, pid %d) while trying to %s a %s page.\r\nThe address of that page is %p (Page directory index %d, page table index %d).\r\nDumping registers: \r\n"
					"\tEDI: %p\r\n"
					"\tESI: %p\r\n"
					"\tEBP: %p\r\n"
					"\tESP: %p\r\n"
					"\tEBX: %p\r\n"
					"\tEDX: %p\r\n"
					"\tECX: %p\r\n"
					"\tEAX: %p\r\n"
					"\tEIP: %p\r\n"
					"\tEFLAGS: %p\r\n"
					"\tSS: %p\r\n"
					"\tDS: %p\r\n"
					"\tCS: %p\r\n"),
				privilegeLevel, frame->eip, multitasking::GetCurrentThreadTid(), pid, action, isPresent, location,
				memory::PageDirectory::addressToIndex(location), memory::PageDirectory::addressToPageTableIndex(location),
				frame->edi, frame->esi, frame->ebp, frame->esp, frame->ebx,
				frame->edx, frame->ecx, frame->eax, frame->eip, frame->eflags,
				frame->ss, frame->ds, frame->cs);
		asm volatile("cli; hlt");
	}
	void defaultExceptionHandler(const interrupt_frame* frame)
	{
		if (frame->errorCode == 1 && utils::testBitInBitfield(frame->eflags, 8))
		{
			//utils::clearBitInBitfield(reinterpret_cast<UINT32_T>(const_cast<interrupt_frame*>(frame)->eflags), 8);
			return;
		}
		DWORD pid = 0xFFFFFFFF;
		if (multitasking::g_initialized)
		{
			if (multitasking::g_currentThread->owner && multitasking::g_currentThread->owner->isUserMode)
				multitasking::g_currentThread->owner->TerminateProcess(~frame->intNumber);
			pid = multitasking::g_currentThread->owner->pid;
		}
		kpanic((PVOID)frame->ebp, (PVOID)frame->eip,
			kpanic_format(
				"Unhandled exception %d at %p (tid %d, pid %d). Error code: %d. Dumping registers: \r\n"
				"\tEDI: %p\r\n"
				"\tESI: %p\r\n"
				"\tEBP: %p\r\n"
				"\tESP: %p\r\n"
				"\tEBX: %p\r\n"
				"\tEDX: %p\r\n"
				"\tECX: %p\r\n"
				"\tEAX: %p\r\n"
				"\tEIP: %p\r\n"
				"\tEFLAGS: %p\r\n"
				"\tSS: %p\r\n"
				"\tDS: %p\r\n"
				"\tCS: %p\r\n"),
			frame->intNumber,
			frame->eip,
			multitasking::GetCurrentThreadTid(), pid,
			frame->errorCode,
			frame->edi, frame->esi, frame->ebp, frame->esp, frame->ebx,
			frame->edx, frame->ecx, frame->eax, frame->eip, frame->eflags,
			frame->ss, frame->ds, frame->cs);
	}
}