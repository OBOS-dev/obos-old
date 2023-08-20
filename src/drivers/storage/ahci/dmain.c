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

extern PVOID VirtualAlloc(PVOID base, SIZE_T nPages, DWORD flags);

void ReadFromDrive(AHCI_PORT* port, UINTPTR_T offsetSectors, SIZE_T countSectors, UINT16_T* buffer);

void StopCommandEngine(HBA_PORT* port);
void StartCommandEngine(HBA_PORT* port);

PVOID memzero(PVOID blk, SIZE_T count)
{
	for (int i = 0; i < count; ((PBYTE)blk)[i++] = 0x00);
	return blk;
}

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
	if (!(hbaMem->cap & (1 << 18)))
		hbaMem->ghc |= (1 << 31);
	while ((hbaMem->bohc & 1) && !(hbaMem->bohc & (1 << 1)))
		hbaMem->bohc |= (1 << 1);
	hbaMem->bohc |= (1 << 1);

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
	
	// Initialize the ports.

	for (int i = 0; i < 31; i++)
	{
		if (!availablePorts[i].available)
			continue;
		HBA_PORT* port = availablePorts[i].hbaPort;
		StopCommandEngine(port);

		PVOID clbBase = availablePorts[i].clbVirtualAddress = VirtualAlloc(0, 1, ALLOC_FLAGS_WRITE_ENABLED | ALLOC_FLAGS_CACHE_DISABLE);
		
		GetPhysicalAddress(clbBase, (PVOID*)&port->clb);
		port->clbu = 0;
		port->fb = port->clb + 1024;
		port->fbu = 0;

		HBA_CMD_HEADER* cmdHeader = (HBA_CMD_HEADER*)clbBase;
		PVOID ctbaBase = VirtualAlloc((PVOID)((UINTPTR_T)clbBase + 4096), 2, ALLOC_FLAGS_WRITE_ENABLED | ALLOC_FLAGS_CACHE_DISABLE);
		
		memzero(ctbaBase, 8192);

		UINTPTR_T ctbaBasePhys = 0;
		GetPhysicalAddress(ctbaBase, (PVOID*)&ctbaBasePhys);

		for (int i = 0; i < 32; i++)
		{
			cmdHeader[i].prdtl = 8;

			cmdHeader[i].ctba = ctbaBasePhys + (i << 8);
			cmdHeader[i].ctbau = 0;
		}

		StartCommandEngine(port);
	}

	PBYTE buffer = VirtualAlloc(0, 2, ALLOC_FLAGS_WRITE_ENABLED | ALLOC_FLAGS_CACHE_DISABLE);

	ReadFromDrive(availablePorts + 0, 0, 1, (UINT16_T*)buffer);

	sti();

	return 0;
}

#define ATA_CMD_READ_DMA_EX 0x25

void ReadFromDrive(AHCI_PORT* _port, UINTPTR_T offsetSectors, SIZE_T countSectors, UINT16_T* buffer)
{
	HBA_PORT* port = _port->hbaPort;
	
	port->cmd |= 1;

	port->is = (UINT32_T)-1;
	UINT32_T slots = (port->sact | port->ci);
	BYTE slot = 0;
	for (int i = 0; i < 32; i++)
	{
		if ((slots & 1) == 0)
		{
			slot = i;
			break;
		}
		slots >>= 1;
	}
	HBA_CMD_HEADER* cmdHeader = (HBA_CMD_HEADER*)_port->clbVirtualAddress;
	cmdHeader += slot;
	cmdHeader->cfl = sizeof(FIS_REG_H2D) / sizeof(UINT32_T);
	cmdHeader->r = 0;
	cmdHeader->prdtl = (UINT16_T)((countSectors >> 4) + 1);

	HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)((UINTPTR_T)_port->clbVirtualAddress + 4096 + (slot << 8));
	memzero(cmdtbl, sizeof(HBA_CMD_TBL) +
		(cmdHeader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));

	// 8K bytes (16 sectors) per PRDT
	int i = 0;
	for (; i < cmdHeader->prdtl - 1; i++)
	{
		cmdtbl->prdt_entry[i].dba = (UINT32_T)buffer;
		cmdtbl->prdt_entry[i].dbc = 8 * 1024 - 1;	// 8K bytes (this value should always be set to 1 less than the actual value)
		cmdtbl->prdt_entry[i].i = 1;
		buffer += 4 * 1024;	// 4K words
		countSectors -= 16;	// 16 sectors
	}
	// Last entry
	cmdtbl->prdt_entry[i].dba = (UINT32_T)buffer;
	cmdtbl->prdt_entry[i].dbc = (countSectors << 9) - 1;	// 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 1;

	// Setup command
	FIS_REG_H2D* cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);

	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_READ_DMA_EX;

	cmdfis->lba0 = (UINT8_T)offsetSectors;
	cmdfis->lba1 = (UINT8_T)(offsetSectors >> 8);
	cmdfis->lba2 = (UINT8_T)(offsetSectors >> 16);
	cmdfis->device = 1 << 6;	// LBA mode

	cmdfis->lba3 = (UINT8_T)(offsetSectors >> 24);
	cmdfis->lba4 = (UINT8_T)(offsetSectors + countSectors);
	cmdfis->lba5 = (UINT8_T)((offsetSectors + countSectors) >> 8);

	cmdfis->countl = countSectors & 0xFF;
	cmdfis->counth = (countSectors >> 8) & 0xFF;

#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08
#define HBA_PxIS_TFES   (1 << 30)

	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)));

	port->ci = 1 << slot;	// Issue command

	// Wait for completion
	while (1)
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((port->ci & (1 << slot)) == 0)
			break;
		if (port->is & HBA_PxIS_TFES)	// Task file error
		{
			Printf("Couldn't read from the disk.\r\n");
			return;
		}
	}

	// Check again
	if (port->is & HBA_PxIS_TFES)
	{
		Printf("Couldn't read from the disk.\r\n");
		return;
	}
}

#define HBA_PxCMD_ST    0x0001
#define HBA_PxCMD_FRE   0x0010
#define HBA_PxCMD_FR    0x4000
#define HBA_PxCMD_CR    0x8000

void StopCommandEngine(HBA_PORT* port)
{
	port->cmd &= ~HBA_PxCMD_ST;

	// Clear FRE (bit4)
	port->cmd &= ~HBA_PxCMD_FRE;

	// Wait until FR (bit14), CR (bit15) are cleared
	while (1)
	{
		if (port->cmd & HBA_PxCMD_FR)
			continue;
		if (port->cmd & HBA_PxCMD_CR)
			continue;
		break;
	}
}

void StartCommandEngine(HBA_PORT* port)
{
	while (port->cmd & HBA_PxCMD_CR);

	// Set FRE (bit4) and ST (bit0)
	port->cmd |= HBA_PxCMD_FRE;
	port->cmd |= HBA_PxCMD_ST;
}

asm(".intel_syntax noprefix;"
	"VirtualAlloc: ;"
		"lea ebx, [esp+4];"
		"mov eax, 36;"
		"int 0x40;"
		"ret;"
);