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

AHCI_PORT g_availablePorts[31];

extern PVOID VirtualAlloc(PVOID base, SIZE_T nPages, DWORD flags);

void ReadFromDrive(AHCI_PORT* port, UINTPTR_T offsetSectors, UINTPTR_T endSectors, SIZE_T countSectors, UINT16_T* buffer);

void StopCommandEngine(HBA_PORT* port);
void StartCommandEngine(HBA_PORT* port);

PVOID memzero(PVOID blk, SIZE_T count)
{
	for (int i = 0; i < count; ((PBYTE)blk)[i++] = 0x00);
	return blk;
}

void RemapPort(AHCI_PORT* port);

HBA_MEM* g_hbaMem = 0;

int _start()
{
	RegisterDriver(PASS_OBOS_API_PARS DRIVER_ID, SERVICE_TYPE_STORAGE_DEVICE);

	cli();

	UINT8_T bus = 0;
	UINT8_T function = 0;
	UINT8_T slot = 0;

	if (!enumeratePci(0x01, 0x06, 0x01, &slot, &function, &bus))
	{
		Printf(PASS_OBOS_API_PARS "AHCI Fatal Error: No ahci controller!\r\n");
		sti();
		return 1;
	}

	UINTPTR_T baseAddr = pciReadDwordRegister(bus, slot, function, getRegisterOffset(9, 0)) & (~0b1111);

	Printf(PASS_OBOS_API_PARS "AHCI Log: Found ahci controller at pci bus %d, function %d, slot %d. Base address: %p.\r\n", bus, function, slot,
		baseAddr);

	pciWriteDwordRegister(bus, slot, function, getRegisterOffset(9, 0), 0xFFFFFFFF);
	SIZE_T hbaSize = (~pciReadDwordRegister(bus, slot, function, getRegisterOffset(9, 0)) & (~0b1111)) + 1;
	Printf(PASS_OBOS_API_PARS "AHCI Log: The hba base takes up %d bytes.\r\n", hbaSize);
	hbaSize = ((hbaSize >> 12) + 1) << 12;
	pciWriteDwordRegister(bus, slot, function, getRegisterOffset(9, 0), baseAddr);

	for (UINTPTR_T base = 0x410000, virt = 0x410000, phys = baseAddr; virt < (base + hbaSize); virt += 4096, phys += 4096)
		MapPhysicalTo(PASS_OBOS_API_PARS phys, (PVOID)virt, ALLOC_FLAGS_CACHE_DISABLE | ALLOC_FLAGS_WRITE_ENABLED);

	UINT16_T pciCommand = pciReadWordRegister(bus, slot, function, getRegisterOffset(1, 2));
	// DMA Bus mastering, and memory space access are on.
	pciCommand |= 6;
	UINT16_T pciStatus = pciReadWordRegister(bus, slot, function, getRegisterOffset(1, 0));
	pciWriteDwordRegister(bus, slot, function, getRegisterOffset(1, 0), pciStatus | pciCommand);

	g_hbaMem = (PVOID)0x410000;

	g_hbaMem->ghc.hr = true;
	g_hbaMem->ghc.ae = true;
	g_hbaMem->ghc.ie = false;

	g_hbaMem->bohc |= (1 << 1);

	UINT32_T implementedPorts = g_hbaMem->pi;

	// Probe the hba ports and initialize them.
	for (UINT32_T i = 0; i < 32; i++)
	{
		if (!testBit(implementedPorts, i))
			continue;
		HBA_PORT* port = &g_hbaMem->ports[i];

		UINT32_T ssts = port->ssts;

		UINT8_T ipm = (ssts >> 8) & 0x0F;
		UINT8_T det = ssts & 0x0F;

		if (det != HBA_PORT_DET_PRESENT || ipm != HBA_PORT_IPM_ACTIVE)
		{
			Printf(PASS_OBOS_API_PARS "AHCI Warning: No drive found at port %d, even though it is implemented.\r\n", i);
			continue;
		}

		switch (port->sig)
		{
		case SATA_SIG_ATAPI:
		{
			Printf(PASS_OBOS_API_PARS "AHCI Log: Found satapi drive at port %d.\r\n", i);
			g_availablePorts[i].available = true;
			g_availablePorts[i].driveType = DRIVE_TYPE_SATAPI;
			g_availablePorts[i].hbaPort = port;
			break;
		}
		case SATA_SIG_SEMB:
		{
			Printf(PASS_OBOS_API_PARS "AHCI Log: Found semb drive at port %d.\r\n", i);
			break;
		}
		case SATA_SIG_PM:
		{
			Printf(PASS_OBOS_API_PARS "AHCI Log: Found pm drive at port %d.\r\n", i);
			break;
		}
		default:
		{
			g_availablePorts[i].available = true;
			g_availablePorts[i].driveType = DRIVE_TYPE_SATA;
			g_availablePorts[i].hbaPort = port;
			Printf(PASS_OBOS_API_PARS "AHCI Log: Found sata drive at port %d.\r\n", i);
			break;
		}
		}

		RemapPort(&g_availablePorts[i]);
	}

	PBYTE buffer = VirtualAlloc(0, 2, ALLOC_FLAGS_WRITE_ENABLED | ALLOC_FLAGS_CACHE_DISABLE);

	{
		UINT32_T* buf = (UINT32_T*)buffer;
		for (int i = 0; i < 1024; buf[i++] = 0);
	}

	UINTPTR_T bufferPhys = 0;
	GetPhysicalAddress(PASS_OBOS_API_PARS buffer, (PVOID*)&bufferPhys);

	AHCI_PORT* port = &g_availablePorts[0];

	for (int i = 0; i < 32; i++)
	{
		if (g_availablePorts[i].available && g_availablePorts[i].driveType == DRIVE_TYPE_SATA)
		{
			port = g_availablePorts + i;
			break;
		}
	}

	ReadFromDrive(port, 0, 1, 1, (UINT16_T*)bufferPhys);

	Printf(PASS_OBOS_API_PARS "Hexdump of disk sector offset 0:15.\r\n");

	for (int i = 0; i < 512; i++)
		Printf(PASS_OBOS_API_PARS "%x%s", buffer[i] & 0xFF, (!(i % 32) && i) ? "\r\n" : " ");

	Printf(PASS_OBOS_API_PARS "\r\n");

	sti();

	return 0;
}

#define ATA_CMD_READ_DMA_EX 0x25
#define ATA_DEV_BUSY 0x80
#define ATA_DEV_DRQ 0x08

void ReadFromDrive(AHCI_PORT* _port, UINTPTR_T startl, UINTPTR_T starth, SIZE_T count, UINT16_T* buf)
{
	HBA_PORT* port = _port->hbaPort;

	StopCommandEngine(port);

	port->is = (UINT32_T)-1;		// Clear pending interrupt bits
	int spin = 0; // Spin lock timeout counter
	int find_cmdslot(HBA_PORT * port);
	int slot = find_cmdslot(port);
	if (slot == -1)
		return;

	HBA_CMD_HEADER* cmdheader = (HBA_CMD_HEADER*)_port->clbVirtualAddress;
	cmdheader += slot;
	cmdheader->cfl = sizeof(FIS_REG_H2D) / sizeof(UINT32_T);	// Command FIS size
	cmdheader->w = 0;		// Read from device
	cmdheader->prdtl = (UINT16_T)((count - 1) >> 4) + 1;	// PRDT entries count

	HBA_CMD_TBL* cmdtbl = (HBA_CMD_TBL*)((UINTPTR_T)(((UINTPTR_T)cmdheader->ctba << 32) | cmdheader->ctbau));
	{
		UINTPTR_T phys = 0;
		GetPhysicalAddress(PASS_OBOS_API_PARS _port->clbVirtualAddress, (PVOID*)&phys);
		cmdtbl = (HBA_CMD_TBL*)((PBYTE)_port->clbVirtualAddress + (cmdheader->ctba - phys));
	}
	memzero(cmdtbl, sizeof(HBA_CMD_TBL) +
		(cmdheader->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));

	int i = 0;

	// 8K bytes (16 sectors) per PRDT
	for (; i < cmdheader->prdtl - 1; i++)
	{
		cmdtbl->prdt_entry[i].dba = (UINTPTR_T)buf;
		cmdtbl->prdt_entry[i].dbc = 8 * 1024 - 1;	// 8K bytes (this value should always be set to 1 less than the actual value)
		cmdtbl->prdt_entry[i].i = 1;
		buf += 4 * 1024;	// 4K words
		count -= 16;	// 16 sectors
	}
	// Last entry
	cmdtbl->prdt_entry[i].dba = (UINTPTR_T)buf;
	cmdtbl->prdt_entry[i].dbc = (count << 9) - 1;	// 512 bytes per sector
	cmdtbl->prdt_entry[i].i = 1;

	// Setup command
	FIS_REG_H2D* cmdfis = (FIS_REG_H2D*)(&cmdtbl->cfis);

	cmdfis->fis_type = FIS_TYPE_REG_H2D;
	cmdfis->c = 1;	// Command
	cmdfis->command = ATA_CMD_READ_DMA_EX;

	cmdfis->lba0 = (UINT8_T)startl;
	cmdfis->lba1 = (UINT8_T)(startl >> 8);
	cmdfis->lba2 = (UINT8_T)(startl >> 16);
	cmdfis->device = 1 << 6;	// LBA mode

	cmdfis->lba3 = (UINT8_T)(startl >> 24);
	cmdfis->lba4 = (UINT8_T)starth;
	cmdfis->lba5 = (UINT8_T)(starth >> 8);

	/*cmdfis->countl = count & 0xFF;
	cmdfis->counth = (count >> 8) & 0xFF;*/
	cmdfis->countl = 0;
	cmdfis->counth = 0;

	// The below loop waits until the port is no longer busy before issuing a new command
	while ((port->tfd & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spin < 1000000)
	{
		spin++;
	}
	if (spin == 1000000)
	{
		Printf(PASS_OBOS_API_PARS "Port is hung\n");
		return;
	}

	StartCommandEngine(port);

	port->ci = 1 << slot;	// Issue command

#define HBA_PxIS_TFES (1 << 30)

	// Wait for completion
	while (1)
	{
		// In some longer duration reads, it may be helpful to spin on the DPS bit 
		// in the PxIS port field as well (1 << 5)
		if ((port->ci & (1 << slot)) == 0)
			break;
		if (port->is & HBA_PxIS_TFES)	// Task file error
		{
			Printf(PASS_OBOS_API_PARS "Read disk error\r\n");
			return;
		}
	}

	// Check again
	if (port->is & HBA_PxIS_TFES)
	{
		Printf(PASS_OBOS_API_PARS "Read disk error\r\n");
		return;
	}

	return;
}

// Find a free command list slot
int find_cmdslot(HBA_PORT* port)
{
	// If not set in SACT and CI, the slot is free
	UINT32_T slots = (port->sact | port->ci);
	for (int i = 0; i < g_hbaMem->cap.nsc; i++)
	{
		if ((slots & 1) == 0)
			return i;
		slots >>= 1;
	}
	Printf(PASS_OBOS_API_PARS "Cannot find free command list entry\n");
	return -1;
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

void RemapPort(AHCI_PORT* _port)
{
	HBA_PORT* port = _port->hbaPort;

	// Wait until FR (bit14), CR (bit15) are cleared
	while (1)
	{
		if (port->cmd & HBA_PxCMD_FR)
			continue;
		if (port->cmd & HBA_PxCMD_CR)
			continue;
		break;
	}

	port->cmd &= ~(1 << 15);
	port->cmd &= ~(1 << 14);
	port->cmd &= ~(1 << 0);
	port->cmd &= ~(1 << 4);

	if (!g_hbaMem->cap.sss)
		port->cmd |= (1 << 1);

	PVOID clbBase = _port->clbVirtualAddress = VirtualAlloc(0, 1, ALLOC_FLAGS_WRITE_ENABLED | ALLOC_FLAGS_CACHE_DISABLE);
	if (!clbBase)
	{
		Printf(PASS_OBOS_API_PARS "AHCI Fatal Error: VirtualAlloc returned nullptr\r\n.");
		return;
	}
	UINTPTR_T clbBasePhys = 0;

	port->fbs &= ~(1 << 0);
	GetPhysicalAddress(PASS_OBOS_API_PARS clbBase, (PVOID*)&clbBasePhys);
	port->clb = clbBasePhys & ~(0b1111111111);
	port->clbu = 0;
	port->fb = clbBasePhys + 1024;
	port->fbu = 0;

	port->serr = 0xFFFFFFFF;
	port->is = 0;
	port->ie = 0;

	HBA_CMD_HEADER* cmdHeader = (HBA_CMD_HEADER*)clbBase;
	PVOID ctbaBase = VirtualAlloc((PVOID)((UINTPTR_T)clbBase + 4096), 2, ALLOC_FLAGS_WRITE_ENABLED | ALLOC_FLAGS_CACHE_DISABLE);

	memzero(ctbaBase, 8192);

	UINTPTR_T ctbaBasePhys = 0;
	GetPhysicalAddress(PASS_OBOS_API_PARS ctbaBase, (PVOID*)&ctbaBasePhys);

	for (int i = 0; i < g_hbaMem->cap.nsc; i++)
	{
		cmdHeader[i].prdtl = 8;

		cmdHeader[i].ctba = ctbaBasePhys + (i << 7);
		cmdHeader[i].ctbau = 0;
	}

	// Wait for PxCMD.CR to be clear.
	while (port->cmd & (1<<15));

	// PxCMD.FRE (FIS Receive Enable)
	port->cmd |= (1 << 4);
	// PxCMD.ST (Start)
	port->cmd |= (1 << 0);

	port->is = 0;
	port->ie = 0xffffffff;
}

asm(".intel_syntax noprefix;"
	"VirtualAlloc: ;"
		"lea ebx, [esp+4];"
		"mov eax, 36;"
		"int 0x40;"
		"ret;"
);