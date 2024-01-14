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

#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>

#include <vfs/devManip/memcpy.h>

#include "structs.h"
#include "command.h"

namespace obos
{
	namespace memory
	{
		OBOS_EXPORT void* MapPhysicalAddress(PageMap* pageMap, uintptr_t phys, void* to, uintptr_t cpuFlags);
		OBOS_EXPORT void UnmapAddress(PageMap* pageMap, void* _addr);
		OBOS_EXPORT uintptr_t DecodeProtectionFlags(uintptr_t _flags);
	}
}

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
    size_t pagesToRead = (nSectorsToRead * portDescriptor.sectorSize) / 4096;
	if (((nSectorsToRead * portDescriptor.sectorSize) % 4096) != 0)
        pagesToRead++;
    size_t blocksToRead = (pagesToRead / 1024) + ((pagesToRead % 1024) != 0);
    uintptr_t *responsePhysicalAddresses = new uintptr_t[blocksToRead];
    uintptr_t responsePhysicalAddressBase = memory::allocatePhysicalPage(pagesToRead);
    for (size_t i = 0; i < blocksToRead; i++)
        responsePhysicalAddresses[i] = responsePhysicalAddressBase + i * 1024;
    StopCommandEngine(pPort);
    size_t currentSectorCount = nSectorsToRead;
    size_t currentPageCount = pagesToRead;
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
	    	 ((cmdHeader->ctba | ((uintptr_t)cmdHeader->ctbau << 32)) /* physical address of the HBA_CMD_TBL */ & 0xfff) // The offset of the HBA_CMD_TBL
	    );
    for (size_t i = 0; i < blocksToRead; i++)
    {
	    uintptr_t responsePhys = responsePhysicalAddresses[i];
	    // Setup the command slot.
	    cmdTBL->prdt_entry[0].dba = responsePhys & 0xffffffff;
	    if (g_generalHostControl->cap.s64a)
	    	cmdTBL->prdt_entry[0].dbau = responsePhys >> 32;
	    else
	    	if (responsePhys >> 32)
	    		logger::panic(nullptr, "AHCI: %s: responsePhys has its upper 32-bits set and cap.s64a is false.\n", __func__);
	    cmdTBL->prdt_entry[0].dbc = currentPageCount * 4096 - 1;
	    cmdTBL->prdt_entry[0].i = 1;
	    FIS_REG_H2D* command = (FIS_REG_H2D*)&cmdTBL->cfis;
	    utils::memzero(command, sizeof(*command));
	    command->fis_type = FIS_TYPE_REG_H2D;
	    command->command = ATA_READ_DMA_EXT;
	    command->device = 0x40;
	    command->c = 1;
        uint16_t count = i == (blocksToRead - 1) ? currentSectorCount : 1024;
	    command->countl = (uint8_t)(count & 0xff);
	    command->counth = (uint8_t)((count >> 8) & 0xff);
        command->lba0 = (uint8_t) (currentLBAOffset & 0xff);
        command->lba1 = (uint8_t)((currentLBAOffset >>  8) & 0xff);
        command->lba2 = (uint8_t)((currentLBAOffset >> 16) & 0xff);
        command->lba3 = (uint8_t)((currentLBAOffset >> 24) & 0xff);
        command->lba4 = (uint8_t)((currentLBAOffset >> 32) & 0xff);
        command->lba5 = (uint8_t)((currentLBAOffset >> 48) & 0xff);
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
        if (currentPageCount > 1024)
            currentPageCount -= 1024;
        currentLBAOffset += (nSectorsToRead - currentSectorCount);
    }
    if (oNSectorsRead)
        *oNSectorsRead = nSectorsToRead;
    if (buff)
    {
        memory::PageMap* currentPageMap = memory::getCurrentPageMap();
        byte* response = (byte*)memory::_Impl_FindUsableAddress(nullptr, pagesToRead);
        size_t i = 0;
        for (byte* addr = response; addr < (response + pagesToRead * 4096); addr += 4096, i++)
            memory::MapPhysicalAddress(
                currentPageMap,
                responsePhysicalAddressBase + i * 4096,
                addr,
                memory::DecodeProtectionFlags(0)|1
            );
        *buff = response;
    }
    delete[] responsePhysicalAddresses;
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
    Port* _portDescriptor = GetPortDescriptorFromKernelDriveID(driveId);
    if (!_portDescriptor)
        return false;
    Port& portDescriptor = *_portDescriptor;
    volatile HBA_PORT* pPort = portDescriptor.hbaPort;
    if ((lbaOffset + nSectorsToWrite) > portDescriptor.nSectors)
    {
        if (oNSectorsWrote)
            *oNSectorsWrote = 0;
        return true;
    }
    // Based on nSectorsToRead and portDescriptor.sectorSize, we might need to send multiple READ commands to get all our data.
    memory::VirtualAllocator vallocator{ nullptr };
    size_t pagesToWrite = (nSectorsToWrite * portDescriptor.sectorSize) / 4096;
	if (((nSectorsToWrite * portDescriptor.sectorSize) % 4096) != 0)
        pagesToWrite++;
    size_t blocksToWrite = (pagesToWrite / 1024) + ((pagesToWrite % 1024) != 0);
    uintptr_t *dataPhysicalAddresses = new uintptr_t[blocksToWrite];
    uintptr_t dataPhysicalAddressBase = memory::allocatePhysicalPage(pagesToWrite);
    for (size_t i = 0; i < blocksToWrite; i++)
        dataPhysicalAddresses[i] = dataPhysicalAddressBase + i * 1024;
    {
        memory::PageMap* currentPageMap = memory::getCurrentPageMap();
        byte* data = (byte*)memory::_Impl_FindUsableAddress(nullptr, pagesToWrite);
        size_t i = 0;
        for (byte* addr = data; addr < (data + pagesToWrite * 4096); addr += 4096, i++)
            memory::MapPhysicalAddress(
                currentPageMap,
                dataPhysicalAddressBase + i * 4096,
                addr,
                memory::DecodeProtectionFlags(0)|1
            );
        // Should always exist on x86-64, so we're good using it without the check.
        vfs::_VectorizedMemcpy64B(data, buff, (nSectorsToWrite * portDescriptor.sectorSize) / 64);
        i = 0;
        for (byte* addr = data; addr < (data + pagesToWrite * 4096); addr += 4096, i++)
            memory::UnmapAddress(currentPageMap, addr);
    }
    StopCommandEngine(pPort);
    size_t currentSectorCount = nSectorsToWrite;
    size_t currentPageCount = pagesToWrite;
    size_t currentLBAOffset = lbaOffset;
	uint32_t cmdSlot = FindCMDSlot(pPort);
    HBA_CMD_HEADER* cmdHeader = (HBA_CMD_HEADER*)portDescriptor.clBase + cmdSlot;
	cmdHeader->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
	cmdHeader->w   = 1; // Host to Device.
	cmdHeader->prdtl = 1; // One PRDT entry
    HBA_CMD_TBL* cmdTBL = 
	    (HBA_CMD_TBL*)
	    (
	    	(byte*)portDescriptor.clBase +
	    	 ((cmdHeader->ctba | ((uintptr_t)cmdHeader->ctbau << 32)) /* physical address of the HBA_CMD_TBL */ - portDescriptor.clBasePhys) // The offset of the HBA_CMD_TBL
	    );
    for (size_t i = 0; i < blocksToWrite; i++)
    {
	    uintptr_t dataPhys = dataPhysicalAddresses[i];
	    // Setup the command slot.
	    cmdTBL->prdt_entry[0].dba = dataPhys & 0xffffffff;
	    if (g_generalHostControl->cap.s64a)
	    	cmdTBL->prdt_entry[0].dbau = dataPhys >> 32;
	    else
	    	if (dataPhys >> 32)
	    		logger::panic(nullptr, "AHCI: %s: dataPhys has its upper 32-bits set and cap.s64a is false.\n", __func__);
	    cmdTBL->prdt_entry[0].dbc = currentPageCount * 4096 - 1;
	    cmdTBL->prdt_entry[0].i = 1;
	    FIS_REG_H2D* command = (FIS_REG_H2D*)&cmdTBL->cfis;
	    utils::memzero(command, sizeof(*command));
	    command->fis_type = FIS_TYPE_REG_H2D;
	    command->command = ATA_WRITE_DMA_EXT;
	    command->device = 0;
	    command->c = 1;
        uint16_t count = i == (blocksToWrite - 1) ? currentSectorCount : 1024;
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
        if (currentPageCount > 1024)
            currentPageCount -= 1024;
        currentLBAOffset += (nSectorsToWrite - currentSectorCount);
    }
    if (oNSectorsWrote)
        *oNSectorsWrote = nSectorsToWrite;
    for (uintptr_t _phys = dataPhysicalAddressBase; _phys < (_phys + (pagesToWrite * 4096)); _phys += 4096)
        memory::freePhysicalPage(_phys);
    delete[] dataPhysicalAddresses;
    return true;
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