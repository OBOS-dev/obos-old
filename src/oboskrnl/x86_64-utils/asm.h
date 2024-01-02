/*
	oboskrnl/x86_64-utils/asm.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

namespace obos
{
	OBOS_EXPORT void outb(uint16_t port, uint8_t data);
	OBOS_EXPORT void outw(uint16_t port, uint16_t data);
	OBOS_EXPORT void outd(uint16_t port, uint32_t data);
	OBOS_EXPORT uint8_t inb(uint16_t port);
	OBOS_EXPORT uint16_t inw(uint16_t port);
	OBOS_EXPORT uint32_t ind(uint16_t port);

	OBOS_EXPORT void cli();
	OBOS_EXPORT void sti();
	OBOS_EXPORT void hlt();

	OBOS_EXPORT void pause();
 	
	OBOS_EXPORT uintptr_t saveFlagsAndCLI();
	OBOS_EXPORT void restorePreviousInterruptStatus(uintptr_t flags);

	OBOS_EXPORT void* getCR2();
	OBOS_EXPORT uintptr_t getCR0();
	OBOS_EXPORT uintptr_t getCR4();
	OBOS_EXPORT void setCR4(uintptr_t val);
	OBOS_EXPORT uintptr_t getCR8();
	OBOS_EXPORT void setCR8(uintptr_t _cr8);
	OBOS_EXPORT uintptr_t getEFER();
	OBOS_EXPORT void invlpg(uintptr_t addr);

	OBOS_EXPORT uint64_t rdmsr(uint32_t addr);
	OBOS_EXPORT void wrmsr(uint32_t addr, uint64_t val);

	OBOS_EXPORT void __cpuid__(uint64_t initialEax, uint64_t initialEcx, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx);

	OBOS_EXPORT uint64_t rdtsc();

	OBOS_EXPORT // If *val1 == nullptr, val1 = val2.
	OBOS_EXPORT void set_if_zero(uint64_t* val1, uint64_t val2);

	OBOS_EXPORT [[noreturn]] void haltCPU(); // cli(); while(1) hlt();
	OBOS_EXPORT [[noreturn]] void infiniteHLT(); // sti(); while(1) hlt();

	OBOS_EXPORT void int1();
	OBOS_EXPORT void int3();

	OBOS_EXPORT uint64_t bswap64(uint64_t val);
	OBOS_EXPORT uint32_t bswap32(uint32_t val);

	OBOS_EXPORT uint64_t getDR0(void);
	OBOS_EXPORT uint64_t getDR1(void);
	OBOS_EXPORT uint64_t getDR2(void);
	OBOS_EXPORT uint64_t getDR3(void);
	OBOS_EXPORT uint64_t getDR6(void);
	OBOS_EXPORT uint64_t getDR7(void);

	OBOS_EXPORT void setDR0(uint64_t val);
	OBOS_EXPORT void setDR1(uint64_t val);
	OBOS_EXPORT void setDR2(uint64_t val);
	OBOS_EXPORT void setDR3(uint64_t val);
	OBOS_EXPORT void setDR6(uint64_t val);
	OBOS_EXPORT void setDR7(uint64_t val);

	OBOS_EXPORT void *getRBP();

	OBOS_EXPORT uint8_t bsf(uint32_t bitfield);
	OBOS_EXPORT uint8_t bsf(uint64_t bitfield);
	OBOS_EXPORT uint8_t bsf(__uint128_t bitfield);
	OBOS_EXPORT uint8_t bsr(uint32_t bitfield);
	OBOS_EXPORT uint8_t bsr(uint64_t bitfield);
	OBOS_EXPORT uint8_t bsr(__uint128_t bitfield);
}