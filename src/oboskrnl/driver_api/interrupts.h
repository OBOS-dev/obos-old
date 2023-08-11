/*
	driver_api/interrupts.h

	Copyright (c) 2023 Omar Berrow
*/

// Defines the driver syscall functions.
// This is not to be used outside of the kernel

#pragma once

#include <types.h>

#include <descriptors/idt/idt.h>

// driver_api/interrupts.asm
extern "C" void isr80();

namespace obos
{
	namespace driverAPI
	{
		extern UINTPTR_T g_syscallTable[256];
		void ResetSyscallHandlers();
		void RegisterSyscallHandler(BYTE syscall, UINTPTR_T address);
		void UnregisterSyscallHandler(BYTE syscall);
	}
}