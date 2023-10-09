#pragma once

#include <types.h>

#define cli() asm volatile("cli")
#define sti() asm volatile("sti")

void outb(UINT16_T port, UINT8_T val);
void outw(UINT16_T port, UINT16_T val);
UINT8_T inb(UINT16_T port);
UINT16_T inw(UINT16_T port);