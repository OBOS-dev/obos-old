/*
	drivers/x86_64/common/enumerate_pci.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>

#include <driverInterface/x86_64/enumerate_pci.h>

#include <x86_64-utils/asm.h>

namespace obos
{
	namespace driverInterface
	{
        uint8_t pciReadByteRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
        {
            uint32_t address;
            uint32_t lbus = (uint32_t)bus;
            uint32_t lslot = (uint32_t)slot;
            uint32_t lfunc = (uint32_t)func;

            address = (uint32_t)((lbus << 16) | (lslot << 11) |
                (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

            outd(0xCF8, address);

            uint8_t ret = (uint16_t)((ind(0xCFC) >> ((offset & 2) * 8)) & 0xFFFFFF);
            return ret;
        }
        uint16_t pciReadWordRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
        {
            uint32_t address;
            uint32_t lbus = (uint32_t)bus;
            uint32_t lslot = (uint32_t)slot;
            uint32_t lfunc = (uint32_t)func;

            address = (uint32_t)((lbus << 16) | (lslot << 11) |
                (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

            outd(0xCF8, address);

            uint16_t ret = (uint16_t)((ind(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
            return ret;
        }
        uint32_t pciReadDwordRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset)
        {
            uint32_t address;
            uint32_t lbus = (uint32_t)bus;
            uint32_t lslot = (uint32_t)slot;
            uint32_t lfunc = (uint32_t)func;

            address = (uint32_t)((lbus << 16) | (lslot << 11) |
                (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

            outd(0xCF8, address);

            return ((ind(0xCFC) >> ((offset & 2) * 8)));
        }

        void pciWriteDwordRegister(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t data)
        {
            uint32_t address;
            uint32_t lbus = (uint32_t)bus;
            uint32_t lslot = (uint32_t)slot;
            uint32_t lfunc = (uint32_t)func;

            address = (uint32_t)((lbus << 16) | (lslot << 11) |
                (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));

            outd(0xCF8, address);
            outd(0xCFC, data);
        }

        typedef struct __commonPciHeader
        {
            uint16_t deviceId;
            uint16_t vendorId;
            uint16_t status;
            uint16_t command;
            uint8_t classCode;
            uint8_t subclass;
            uint8_t progIF;
            uint8_t revisionId;
            uint8_t bist;
            uint8_t headerType;
            uint8_t latencyTimer;
            uint8_t cacheLineSize;
        } commonPciHeader;

		void enumerateBus(
            uint8_t bus, 
            bool(*callback)(
				void* userData, uint8_t currentSlot, uint8_t currentFunction, uint8_t currentBus, uint8_t classCode,
				uint8_t subclass,
				uint8_t progIF),
            void* callbackUserdata)
        {
            int16_t currentSlot = 0;
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
                    uint32_t tmp = pciReadDwordRegister(bus, currentSlot, function, 8);
                    uint8_t classCode = ((uint8_t*)&tmp)[3];
                    uint8_t subclass = ((uint8_t*)&tmp)[2];
                    uint8_t progIF = ((uint8_t*)&tmp)[1];
                    if (!callback(callbackUserdata, currentSlot, function, bus, classCode, subclass, progIF))
                        break;
                    if(!function)
                        if ((pciReadByteRegister(bus, currentSlot, 0, 15) & 0x80) != 0x80)
                            break;
                }
            }
        }
        bool enumerateBus(uint8_t bus, uint8_t expectedClassCode, uint8_t expectedSubclass, uint8_t exceptedProgIF, uint8_t* slot, uint8_t* _function)
        {
            // We're looking for pci device class code 01, subclass 06, prog if 01 (Mass storage controller, serial ata controller, ahci 1.0).
            int16_t currentSlot = 0;
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
                    uint32_t tmp = pciReadDwordRegister(bus, currentSlot, function, 8);
                    uint8_t classCode = ((uint8_t*)&tmp)[3];
                    uint8_t subclass = ((uint8_t*)&tmp)[2];
                    uint8_t progIF = ((uint8_t*)&tmp)[1];
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
        bool enumeratePci(uint8_t expectedClassCode, uint8_t expectedSubclass, uint8_t exceptedProgIF, uint8_t* slot, uint8_t* function, uint8_t* bus)
        {
            int16_t currentBus = 0;
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
    }
}