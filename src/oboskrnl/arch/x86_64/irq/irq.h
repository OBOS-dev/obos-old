/*
	oboskrnl/arch/x86_64/irq/irq.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	void InitializeIrq();
	void SendEOI();

	struct LAPIC
	{
		alignas(0x10) const uint8_t resv1[0x20];
		alignas(0x10) uint32_t lapicID;
		alignas(0x10) uint32_t lapicVersion;
		alignas(0x10) const uint8_t resv2[0x40];
		alignas(0x10) uint32_t taskPriority;
		alignas(0x10) const uint32_t arbitrationPriority;
		alignas(0x10) const uint32_t processorPriority;
		alignas(0x10) uint32_t eoi; // write zero to send eoi.
		alignas(0x10) uint32_t remoteRead;
		alignas(0x10) uint32_t logicalDestination;
		alignas(0x10) uint32_t destinationFormat;
		alignas(0x10) uint32_t spuriousInterruptVector;
		alignas(0x10) const uint32_t inService0_31;
		alignas(0x10) const uint32_t inService32_63;
		alignas(0x10) const uint32_t inService64_95;
		alignas(0x10) const uint32_t inService96_127;
		alignas(0x10) const uint32_t inService128_159;
		alignas(0x10) const uint32_t inService160_191;
		alignas(0x10) const uint32_t inService192_223;
		alignas(0x10) const uint32_t inService224_255;
		alignas(0x10) const uint32_t triggerMode0_31;
		alignas(0x10) const uint32_t triggerMode32_63;
		alignas(0x10) const uint32_t triggerMode64_95;
		alignas(0x10) const uint32_t triggerMode96_127;
		alignas(0x10) const uint32_t triggerMode128_159;
		alignas(0x10) const uint32_t triggerMode160_191;
		alignas(0x10) const uint32_t triggerMode192_223;
		alignas(0x10) const uint32_t triggerMode224_255;
		alignas(0x10) const uint32_t interruptRequest0_31;
		alignas(0x10) const uint32_t interruptRequest32_63;
		alignas(0x10) const uint32_t interruptRequest64_95;
		alignas(0x10) const uint32_t interruptRequest96_127;
		alignas(0x10) const uint32_t interruptRequest128_159;
		alignas(0x10) const uint32_t interruptRequest160_191;
		alignas(0x10) const uint32_t interruptRequest192_223;
		alignas(0x10) const uint32_t interruptRequest224_255;
		alignas(0x10) uint32_t errorStatus;
		alignas(0x10) const uint8_t resv3[0x60];
		alignas(0x10) uint32_t lvtCMCI;
		alignas(0x10) uint32_t interruptCommand0_31;
		alignas(0x10) uint32_t interruptCommand32_63;
		alignas(0x10) uint32_t lvtTimer;
		alignas(0x10) uint32_t lvtThermalSensor;
		alignas(0x10) uint32_t lvtPerformanceMonitoringCounters;
		alignas(0x10) uint32_t lvtLINT0;
		alignas(0x10) uint32_t lvtLINT1;
		alignas(0x10) uint32_t lvtError;
		alignas(0x10) uint32_t initialCount;
		alignas(0x10) const uint32_t currentCount;
		alignas(0x10) const uint8_t resv4[0x40];
		alignas(0x10) uint32_t divideConfig;
		alignas(0x10) const uint8_t resv5[0x10];
	};
	static_assert(sizeof(LAPIC) == 0x400, "struct LAPIC has an invalid size.");
	extern volatile LAPIC* g_localAPICAddr;
}