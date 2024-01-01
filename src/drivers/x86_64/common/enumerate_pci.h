/*
	drivers/x86_64/common/enumerate_pci.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

void outb(uint16_t port, uint8_t out);
void outw(uint16_t port, uint16_t out);
void outl(uint16_t port, uint32_t out);
uint8_t  inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t inl(uint16_t port);

#define getRegisterOffset(reg, offset) ((uint8_t)(reg * 4 + offset))

uint8_t  pciReadByteRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pciReadWordRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint32_t pciReadDwordRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pciWriteDwordRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data);
bool enumerateBus(uint8_t bus, uint8_t expectedClassCode, uint8_t expectedSubclass, uint8_t exceptedProgIF, uint8_t* slot, uint8_t* function);
bool enumeratePci(uint8_t expectedClassCode, uint8_t expectedSubclass, uint8_t exceptedProgIF, uint8_t* slot, uint8_t* function, uint8_t* bus);