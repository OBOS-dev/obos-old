/*
	inline-asm.h

	Copyright (c) 2023 Omar Berrow
*/

#ifndef __OBOS_INLINE_ASM_H
#define __OBOS_INLINE_ASM_H

#include "types.h"

#define nop() asm volatile ("nop")

void outb(UINT16_T port, UINT8_T val);
void outw(UINT16_T port, UINT16_T val);
UINT8_T inb(UINT16_T port);
UINT16_T inw(UINT16_T port);
void io_wait(void);
UINT32_T getEflags();

void cli();
void sti();
void hlt();

void _int(BYTE interrupt);

namespace obos
{
	void RestartComputer();
	// Disables interrupts and makes sure they can't be enabled until LeaveKernelSection is called.
	void EnterKernelSection();
	// Enables interrupts if this call matches the first call to EnterKernelSection. (Like a stack)
	void LeaveKernelSection();
	// Sets g_currentThread->isServicingSyscall to true
	void EnterSyscall();
	// Sets g_currentThread->isServicingSyscall to false
	void ExitSyscall();
}

#endif