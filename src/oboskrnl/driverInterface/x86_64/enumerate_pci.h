/*
	drivers/x86_64/common/enumerate_pci.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

#define PCI_GetRegisterOffset(reg, offset) ((uint8_t)(reg * 4 + offset))

namespace obos
{
	namespace driverInterface
	{
		OBOS_EXPORT uint8_t  pciReadByteRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
		OBOS_EXPORT uint16_t pciReadWordRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
		OBOS_EXPORT uint32_t pciReadDwordRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
		OBOS_EXPORT void pciWriteByteRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t data);
		OBOS_EXPORT void pciWriteWordRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t data);
		OBOS_EXPORT void pciWriteDwordRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data);
		OBOS_EXPORT void enumerateBus(
			uint8_t bus,
			bool(*callback)(
				void* userData, uint8_t currentSlot, uint8_t currentFunction, uint8_t currentBus, uint8_t classCode,
				uint8_t subclass,
				uint8_t progIF),
			void* callbackUserdata);
		OBOS_EXPORT bool enumerateBus(uint8_t bus, uint8_t expectedClassCode, uint8_t expectedSubclass, uint8_t exceptedProgIF, uint8_t* slot, uint8_t* function);
		OBOS_EXPORT bool enumeratePci(uint8_t expectedClassCode, uint8_t expectedSubclass, uint8_t exceptedProgIF, uint8_t* slot, uint8_t* function, uint8_t* bus);
	}
}