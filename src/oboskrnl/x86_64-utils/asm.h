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
	
	uintptr_t saveFlagsAndCLI();
	void restorePreviousInterruptStatus(uintptr_t flags);

	void* getCR2();
	uintptr_t getEFER();
	void invlpg(uintptr_t addr);

	uint64_t rdmsr(uint32_t addr);
	void wrmsr(uint32_t addr, uint64_t val);

	void __cpuid__(uint64_t initialEax, uint64_t initialEcx, uint64_t* eax, uint64_t* ebx, uint64_t* ecx, uint64_t* edx);

	uint64_t rdtsc();

	void atomic_set(bool* val);
	bool atomic_test(bool* val);

	[[noreturn]] void haltCPU();
}