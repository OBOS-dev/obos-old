#ifndef __OBOS_INLINE_ASM_H
#define __OBOS_INLINE_ASM_H

#include "types.h"

#define nop() asm volatile ("nop")

void outb(UINT16_T port, UINT8_T val);
void outw(UINT16_T port, UINT16_T val);
UINT8_T inb(UINT16_T port);
UINT16_T inw(UINT16_T port);
void io_wait(void);

void cli();
void sti();

#endif