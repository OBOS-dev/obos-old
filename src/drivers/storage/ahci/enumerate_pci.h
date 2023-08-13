/*
	driver/storage/ahci/enumerate_pci.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

void outb(UINT16_T port, UINT8_T out);
void outw(UINT16_T port, UINT16_T out);
void outl(UINT16_T port, UINT32_T out);
UINT8_T  inb(UINT16_T port);
UINT16_T inw(UINT16_T port);
UINT32_T inl(UINT16_T port);

#define getRegisterOffset(reg, offset) ((UINT8_T)(reg * 4 + offset))

UINT8_T  pciReadByteRegister(UINT8_T bus, UINT8_T slot, UINT8_T func, UINT8_T offset);
UINT16_T pciReadWordRegister(UINT8_T bus, UINT8_T slot, UINT8_T func, UINT8_T offset);
UINT32_T pciReadDwordRegister(UINT8_T bus, UINT8_T slot, UINT8_T func, UINT8_T offset);
VOID pciWriteDwordRegister(UINT8_T bus, UINT8_T slot, UINT8_T func, UINT8_T offset, UINT32_T data);
BOOL enumerateBus(UINT8_T bus, UINT8_T expectedClassCode, UINT8_T expectedSubclass, UINT8_T exceptedProgIF, UINT8_T* slot, UINT8_T* function);
BOOL enumeratePci(UINT8_T expectedClassCode, UINT8_T expectedSubclass, UINT8_T exceptedProgIF, UINT8_T* slot, UINT8_T* function, UINT8_T* bus);