/*
	interrupts.h

	Copyright (c) 2023 Omar Berrow
*/

#ifndef __OBOS_INTERRUPTS_H
#define __OBOS_INTERRUPTS_H

#include "types.h"

typedef struct __registers
{
	UINT32_T ds;
	UINT32_T edi, esi, ebp, esp, ebx, edx, ecx, eax;
	UINT32_T intNumber, errorCode;
	UINT32_T eip, cs, eflags, useresp, ss;
} isr_registers;

void setIdtEntry(UINT8_T index, UINT32_T base, UINT16_T sel, UINT8_T flags);
void initInterrupts();
void enablePICInterrupt(UINT8_T IRQline);
void disablePICInterrupt(UINT8_T IRQline);
void setPICInterruptHandlers(int* interrupts, int nInterrupts, void(*isr)(int interrupt, isr_registers registers));
void resetPICInterruptHandlers(int* interrupts, int nInterrupts);
void setExceptionHandlers(int* interrupts, int nInterrupts, void(*handler)(int interrupt, int ec, isr_registers registers));
void resetExceptionHandlers(int* interrupts, int nInterrupts);

#endif