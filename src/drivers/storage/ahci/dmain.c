/*
	driver/storage/ahci/dmain.c

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>

#include <driver_api/enums.h>
#include <driver_api/syscall_interface.h>

//#define LOG_PREFIX "AHCI Log: "

#include "enumerate_pci.h"
#include "ahciStructures.h"

#define DRIVER_ID (DWORD)2

#define cli() asm volatile("cli")
#define sti() asm volatile("sti")

BOOL testBit(UINT32_T bitfield, UINT8_T bit)
{
	return (bitfield >> bit) & 1;
}

AHCI_PORT availablePorts[31];

void ReadFromDrive(HBA_PORT* port, UINTPTR_T offsetSectors, SIZE_T bufSize, UINT8_T* buffer);

int _start()
{
	RegisterDriver(DRIVER_ID, SERVICE_TYPE_STORAGE_DEVICE);
	
	cli();

	UINT8_T bus = 0;
	UINT8_T function = 0;
	UINT8_T slot = 0;

	if (!enumeratePci(0x01, 0x06, 0x01, &slot, &function, &bus))
	{
		Printf("AHCI Fatal Error: No ahci controller!\r\n");
		sti();
		return 1;
	}

	UINTPTR_T baseAddr = pciReadDwordRegister(bus, slot, function, getRegisterOffset(9, 0));

	Printf("AHCI Log: Found ahci controller at pci bus %d, function %d, slot %d. Base address: %p.\r\n", bus, function, slot,
		baseAddr);

	pciWriteDwordRegister(bus,slot,function,getRegisterOffset(9,0), 0xFFFFFFFF);
	SIZE_T hbaSize = (~pciReadDwordRegister(bus, slot, function, getRegisterOffset(9, 0))) + 1;
	Printf("AHCI Log: The hba base takes up %d bytes.\r\n", hbaSize);
	hbaSize = ((hbaSize >> 12) + 1) << 12;
	pciWriteDwordRegister(bus,slot,function,getRegisterOffset(9,0), baseAddr);

	HBA_MEM* hbaMem = (PVOID)0x410000;

	for(UINTPTR_T base = 0x410000, virt = 0x410000, phys = baseAddr; virt < (base + hbaSize); virt += 4096, phys += 4096)
		MapPhysicalTo(phys, (PVOID)virt, ALLOC_FLAGS_CACHE_DISABLE | ALLOC_FLAGS_WRITE_ENABLED);
	hbaMem->bohc |= 0x00000002;
	hbaMem->ghc |= 0x80000000;

	UINT32_T implementedPorts = hbaMem->pi;

	// Probe the hba ports.
	for (UINT32_T i = 0; i < 32; i++)
	{
		if (!testBit(implementedPorts, i))
			continue;
		HBA_PORT* port = &hbaMem->ports[i];

		UINT32_T ssts = port->ssts;

		UINT8_T ipm = (ssts >> 8) & 0x0F;
		UINT8_T det = ssts & 0x0F;

		if (det != HBA_PORT_DET_PRESENT || ipm != HBA_PORT_IPM_ACTIVE)
		{
			Printf("AHCI Warning: No drive found at port %d, even though it is implemented.\r\n", i);
			continue;
		}

		switch (port->sig)
		{
		case SATA_SIG_ATAPI:
		{
			Printf("AHCI Log: Found satapi drive at port %d.\r\n", i);
			availablePorts[i].available = true;
			availablePorts[i].driveType = DRIVE_TYPE_SATAPI;
			availablePorts[i].hbaPort = port;
			break;
		}
		case SATA_SIG_SEMB:
		{
			Printf("AHCI Log: Found semb drive at port %d.\r\n", i);
			break;
		}
		case SATA_SIG_PM:
		{
			Printf("AHCI Log: Found pm drive at port %d.\r\n", i);
			break;
		}
		default:
		{
			availablePorts[i].available = true;
			availablePorts[i].driveType = DRIVE_TYPE_SATA;
			availablePorts[i].hbaPort = port;
			Printf("AHCI Log: Found sata drive at port %d.\r\n", i);
			break;
		}
		}
	}

	sti();

	return 0;
}

void ReadFromDrive(HBA_PORT* port, UINTPTR_T offsetSectors, SIZE_T bufSize, UINT8_T* buffer)
{

}