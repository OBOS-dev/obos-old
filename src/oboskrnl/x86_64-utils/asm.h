/*
	oboskrnl/x86_64-utils/asm.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

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

	void pause();
	
	uintptr_t saveFlagsAndCLI();
	void restorePreviousInterruptStatus(uintptr_t flags);

	void* getCR2();
	uintptr_t getCR0();
	uintptr_t getCR4();
	void setCR4(uintptr_t val);
	uintptr_t getCR8();
	void setCR8(uintptr_t _cr8);
	uintptr_t getEFER();
	void invlpg(uintptr_t addr);

	uint64_t rdmsr(uint32_t addr);
	void wrmsr(uint32_t addr, uint64_t val);

	void __cpuid__(uint64_t initialEax, uint64_t initialEcx, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx);

	uint64_t rdtsc();

	// If *val1 == nullptr, val1 = val2.
	void set_if_zero(uint64_t* val1, uint64_t val2);

	[[noreturn]] void haltCPU(); // cli(); while(1) hlt();
	[[noreturn]] void infiniteHLT(); // sti(); while(1) hlt();

	void int1();
	void int3();

	uint64_t bswap64(uint64_t val);
	uint32_t bswap32(uint32_t val);

	uint64_t getDR0(void);
	uint64_t getDR1(void);
	uint64_t getDR2(void);
	uint64_t getDR3(void);
	uint64_t getDR6(void);
	uint64_t getDR7(void);

	void setDR0(uint64_t val);
	void setDR1(uint64_t val);
	void setDR2(uint64_t val);
	void setDR3(uint64_t val);
	void setDR6(uint64_t val);
	void setDR7(uint64_t val);
	
	void *getRBP();
}