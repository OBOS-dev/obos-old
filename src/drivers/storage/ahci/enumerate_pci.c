/*
	driver/storage/ahci/enumerate_pci.h

	Copyright (c) 2023 Omar Berrow
*/

#include "enumerate_pci.h"

#include <types.h>

#include <driver_api/syscall_interface.h>

void outb(UINT16_T port, UINT8_T out)
{
    asm volatile ("outb %0, %1" : : "a"(out), "Nd"(port) : "memory");
}
void outw(UINT16_T port, UINT16_T out)
{
    asm volatile ("outw %0, %1" : : "a"(out), "Nd"(port) : "memory");
}
void outl(UINT16_T port, UINT32_T out)
{
    asm volatile ("outl %0, %1" : : "a"(out), "Nd"(port) : "memory");
}
UINT8_T  inb(UINT16_T port)
{
    UINT8_T ret;
    asm volatile ("inb %1, %0"
        : "=a"(ret)
        : "Nd"(port)
        : "memory");
    return ret;
}
UINT16_T inw(UINT16_T port)
{
    UINT16_T ret;
    asm volatile ("inw %1, %0"
        : "=a"(ret)
        : "Nd"(port)
        : "memory");
    return ret;
}
UINT32_T inl(UINT16_T port)
{
    UINT32_T ret;
    asm volatile ("inl %1, %0"
        : "=a"(ret)
        : "Nd"(port)
        : "memory");
    return ret;
}

UINT8_T pciReadByteRegister(UINT8_T bus, UINT8_T slot, UINT8_T func, UINT8_T offset)
{
    UINT32_T address;
    UINT32_T lbus = (UINT32_T)bus;
    UINT32_T lslot = (UINT32_T)slot;
    UINT32_T lfunc = (UINT32_T)func;
    
    address = (UINT32_T)((lbus << 16) | (lslot << 11) |
        (lfunc << 8) | (offset & 0xFC) | ((UINT32_T)0x80000000));

    outl(0xCF8, address);

    UINT8_T ret = (UINT16_T)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFFFF);
    return ret;
}
UINT16_T pciReadWordRegister(UINT8_T bus, UINT8_T slot, UINT8_T func, UINT8_T offset)
{
    UINT32_T address;
    UINT32_T lbus = (UINT32_T)bus;
    UINT32_T lslot = (UINT32_T)slot;
    UINT32_T lfunc = (UINT32_T)func;
    
    address = (UINT32_T)((lbus << 16) | (lslot << 11) |
        (lfunc << 8) | (offset & 0xFC) | ((UINT32_T)0x80000000));

    outl(0xCF8, address);

    UINT16_T ret = (UINT16_T)((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
    return ret;
}
UINT32_T pciReadDwordRegister(UINT8_T bus, UINT8_T slot, UINT8_T func, UINT8_T offset)
{
    UINT32_T address;
    UINT32_T lbus = (UINT32_T)bus;
    UINT32_T lslot = (UINT32_T)slot;
    UINT32_T lfunc = (UINT32_T)func;
    
    address = (UINT32_T)((lbus << 16) | (lslot << 11) |
        (lfunc << 8) | (offset & 0xFC) | ((UINT32_T)0x80000000));

    outl(0xCF8, address);

    return ((inl(0xCFC) >> ((offset & 2) * 8)));
}

VOID pciWriteDwordRegister(UINT8_T bus, UINT8_T slot, UINT8_T func, UINT8_T offset, UINT32_T data)
{
    UINT32_T address;
    UINT32_T lbus = (UINT32_T)bus;
    UINT32_T lslot = (UINT32_T)slot;
    UINT32_T lfunc = (UINT32_T)func;

    address = (UINT32_T)((lbus << 16) | (lslot << 11) |
        (lfunc << 8) | (offset & 0xFC) | ((UINT32_T)0x80000000));

    outl(0xCF8, address);
    outl(0xCFC, data);
}

typedef struct __commonPciHeader
{
    UINT16_T deviceId;
    UINT16_T vendorId;
    UINT16_T status;
    UINT16_T command;
    UINT8_T classCode;
    UINT8_T subclass;
    UINT8_T progIF;
    UINT8_T revisionId;
    UINT8_T bist;
    UINT8_T headerType;
    UINT8_T latencyTimer;
    UINT8_T cacheLineSize;
} commonPciHeader;

BOOL enumerateBus(UINT8_T bus, UINT8_T expectedClassCode, UINT8_T expectedSubclass, UINT8_T exceptedProgIF, UINT8_T* slot, UINT8_T* _function)
{
    // We're looking for pci device class code 01, subclass 06, prog if 01 (Mass storage controller, serial ata controller, ahci 1.0).
    INT16_T currentSlot = 0;
    for (; currentSlot < 256; currentSlot++)
    {
        for (int function = 0; function < 8; function++)
        {
            if (pciReadWordRegister(bus, currentSlot, function, 0) == 0xFFFF)
            {
                if (!function)
                    break;
                continue; // There is no device here.
            }
            UINT32_T tmp = pciReadDwordRegister(bus, currentSlot, function, 8);
            UINT8_T classCode = ((UINT8_T*)&tmp)[3];
            UINT8_T subclass = ((UINT8_T*)&tmp)[2];
            UINT8_T progIF = ((UINT8_T*)&tmp)[1];
#ifdef LOG_PREFIX
            Printf(LOG_PREFIX "Bus %d, Function %d, Slot %d: Class code: %d, subclass: %d, progIF: %d, header type: %d.\r\n",
                bus, function, currentSlot,
                classCode, subclass, progIF,
                pciReadByteRegister(bus, currentSlot, function, 15));
#endif
            if (classCode == expectedClassCode && subclass == expectedSubclass && progIF == exceptedProgIF)
            {
                if (slot)
                    *slot = currentSlot;
                if (function)
                    *_function = function;
                return true; // yay...
            }
            if(!function)
                if ((pciReadByteRegister(bus, currentSlot, 0, 15) & 0x80) != 0x80)
                    break;
        }
    }
    return false;
}
BOOL enumeratePci(UINT8_T expectedClassCode, UINT8_T expectedSubclass, UINT8_T exceptedProgIF, UINT8_T* slot, UINT8_T* function, UINT8_T* bus)
{
    INT16_T currentBus = 0;
    for (; currentBus < 256; currentBus++)
    {
        if (enumerateBus(currentBus, expectedClassCode, expectedSubclass, exceptedProgIF, slot, function))
        {
            *bus = currentBus;
            return true;
        }
    }
    return false;
}