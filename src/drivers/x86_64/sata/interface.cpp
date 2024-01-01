/*
    drivers/x86_64/sata/interface.cpp

    Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>

#include <driverInterface/struct.h>

#include <x86_64-utils/asm.h>

#include <allocators/vmm/arch.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>

#include "structs.h"
#include "command.h"

using namespace obos;

Port* GetPortDescriptorFromKernelDriveID(uint32_t id)
{
    Port* portDescriptor = &g_ports[0];
    for (uint8_t i = 0; i < 32; i++, portDescriptor++)
    {
        if (portDescriptor->driveType == Port::DRIVE_TYPE_INVALID)
            continue;
        if (portDescriptor->kernelID == id)
            return portDescriptor;
        continue;
    }
    return nullptr;
}

bool DriveReadSectors(
	uint32_t driveId,
	uint64_t lbaOffset,
	size_t nSectorsToRead,
	void** buff,
	size_t* oNSectorsRead
	)
{
    Port* _portDescriptor = GetPortDescriptorFromKernelDriveID(driveId);
    if (!_portDescriptor)
        return false;
    Port& portDescriptor = *_portDescriptor;
    volatile HBA_PORT* pPort = portDescriptor.hbaPort;
    if ((lbaOffset + nSectorsToRead) > portDescriptor.nSectors)
    {
        if (oNSectorsRead)
            *oNSectorsRead = 0;
        return true;
    }
    // Based on nSectorsToRead and portDescriptor.sectorSize, we might need to send multiple READ commands to get all our data.
    memory::VirtualAllocator vallocator{ nullptr };
    size_t pagesToRead = ((nSectorsToRead * portDescriptor.sectorSize) / 4096 + 1);
	volatile byte* response = (volatile byte*)vallocator.VirtualAlloc(nullptr, pagesToRead * 4096, memory::PROT_NO_COW_ON_ALLOCATE);
	uintptr_t responsePhysBase = (uintptr_t)memory::getCurrentPageMap()->getL1PageMapEntryAt((uintptr_t)response) & 0xFFFFFFFFFF000;
    size_t currentSectorCount = nSectorsToRead;
    size_t currentLBAOffset = lbaOffset;
	uint32_t cmdSlot = FindCMDSlot(pPort);
    HBA_CMD_HEADER* cmdHeader = (HBA_CMD_HEADER*)portDescriptor.clBase + cmdSlot;
	cmdHeader->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
	cmdHeader->w   = 0; // Device to Host.
	cmdHeader->prdtl = 1; // One PRDT entry
    HBA_CMD_TBL* cmdTBL = 
	    (HBA_CMD_TBL*)
	    (
	    	(byte*)portDescriptor.clBase +
	    	 ((cmdHeader->ctba | ((uintptr_t)cmdHeader->ctbau << 32)) /* physical address of the HBA_CMD_TBL */ - portDescriptor.clBasePhys) // The offset of the HBA_CMD_TBL
	    );
    for (size_t i = 0; i < pagesToRead; i++)
    {
        StopCommandEngine(pPort);
	    uintptr_t responsePhys = responsePhysBase + (i * 4096);
	    // Setup the command slot.
	    cmdTBL->prdt_entry[0].dba = responsePhys & 0xffffffff;
	    if (g_generalHostControl->cap.s64a)
	    	cmdTBL->prdt_entry[0].dbau = responsePhys & ~(0xffffffff);
	    else
	    	if (responsePhys & ~(0xffffffff))
	    		logger::panic(nullptr, "AHCI: %s: responsePhys has its upper 32-bits set and cap.s64a is false.\n", __func__);
	    cmdTBL->prdt_entry[0].dbc = 4095;
	    cmdTBL->prdt_entry[0].i = 1;
	    FIS_REG_H2D* command = (FIS_REG_H2D*)&cmdTBL->cfis;
	    utils::memzero(command, sizeof(*command));
	    command->fis_type = FIS_TYPE_REG_H2D;
	    command->command = ATA_READ_DMA_EXT;
	    command->device = 0;
	    command->c = 1;
        uint16_t count = i == (pagesToRead - 1) ? currentSectorCount : (4096 / portDescriptor.sectorSize);
	    command->countl = (uint8_t)(count & 0xff);
	    command->counth = (uint8_t)((count >> 8) & 0xff);
        command->lba0 = (currentLBAOffset & 0xff);
        command->lba1 = ((currentLBAOffset >> 8) & 0xff);
        command->lba2 = ((currentLBAOffset >> 16) & 0xff);
        command->lba3 = ((currentLBAOffset >> 24) & 0xff);
        command->lba4 = ((currentLBAOffset >> 32) & 0xff);
        command->lba5 = ((currentLBAOffset >> 48) & 0xff);
	    // Wait on the port.
	    // 0x88: ATA_DEV_BUSY | ATA_DEV_DRQ
	    while ((pPort->tfd & 0x88))
	    	pause();
	    // Issue the command
	    pPort->ci = (1 << cmdSlot);
	    // Wait for the command.
	    // TODO: Make this interrupt driven.
	    StartCommandEngine(pPort);
	    while ( (pPort->ci & (1 << cmdSlot)) && !(pPort->is & (0xFD800000)) /*An error in the IS.*/ );
        if (currentSectorCount > (4096 / portDescriptor.sectorSize))
            currentSectorCount -= (4096 / portDescriptor.sectorSize);
        currentLBAOffset += (nSectorsToRead - currentSectorCount);
    }
    if (oNSectorsRead)
        *oNSectorsRead = nSectorsToRead;
    return true;
}
bool DriveWriteSectors(
	uint32_t driveId,
	uint64_t lbaOffset,
	size_t nSectorsToWrite,
	char* buff,
	size_t* oNSectorsWrote
	)
{

}
bool DriveQueryInfo(
	uint32_t driveId,
	uint64_t *oNSectors,
	uint64_t *oBytesPerSector
	)
{
    Port* portDescriptor = GetPortDescriptorFromKernelDriveID(driveId);
    if (!portDescriptor)
        return false;
    if (oNSectors)
        *oNSectors = portDescriptor->nSectors;
    if (oBytesPerSector)
        *oBytesPerSector = portDescriptor->sectorSize;
    return true;
}