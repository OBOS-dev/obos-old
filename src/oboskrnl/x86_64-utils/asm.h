/*
	oboskrnl/x86_64-utils/asm.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <stdint.h>

namespace obos
{
	void outb(uint16_t port, uint8_t data);
	void outw(uint16_t port, uint16_t data);
	void outd(uint16_t port, uint32_t data);
	uint8_t inb(uint16_t port);
	uint16_t inw(uint16_t port);
	uint32_t ind(uint16_t port);

	void cli();
	void sti();
	void hlt();

	void* getCR2();
}