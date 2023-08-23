/*
	driver/storage/nvme/dmain.c

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>

#include <driver_api/enums.h>
#include <driver_api/syscall_interface.h>

#include "enumerate_pci.h"
#include "nvme.h"

#define DRIVER_ID (DWORD)2

#define cli() asm volatile("cli")
#define sti() asm volatile("sti")

BOOL testBit(UINT32_T bitfield, UINT8_T bit)
{
	return (bitfield >> bit) & 1;
}

extern PVOID VirtualAlloc(PVOID base, SIZE_T nPages, DWORD flags);

PVOID memzero(PVOID blk, SIZE_T count)
{
	for (int i = 0; i < count; ((PBYTE)blk)[i++] = 0x00);
	return blk;
}

NVME_MEM* g_abar = 0;

int _start()
{
	RegisterDriver(DRIVER_ID, SERVICE_TYPE_STORAGE_DEVICE);
	
	return 2;

	cli();

	UINT8_T bus = 0;
	UINT8_T function = 0;
	UINT8_T slot = 0;

	if (!enumeratePci(0x01, 0x08, 0x02, &slot, &function, &bus))
	{
		Printf("NVME Fatal Error: No nvme controller!\r\n");
		sti();
		return 1;
	}

	UINTPTR_T baseAddr = pciReadDwordRegister(bus, slot, function, getRegisterOffset(4, 0)) & (~0b1111);

	Printf("NVME Log: Found nvme controller at pci bus %d, function %d, slot %d. Base address: %p.\r\n", bus, function, slot,
		baseAddr);

	pciWriteDwordRegister(bus,slot,function,getRegisterOffset(4,0), 0xFFFFFFFF);
	SIZE_T hbaSize = (~pciReadDwordRegister(bus, slot, function, getRegisterOffset(4, 0)) & (~0b1111)) + 1;
	Printf("NVME Log: The hba base takes up %d bytes.\r\n", hbaSize);
	hbaSize = ((hbaSize >> 12) + 1) << 12;
	pciWriteDwordRegister(bus,slot,function,getRegisterOffset(4,0), baseAddr);

	UINT16_T pciCommand = pciReadWordRegister(bus, slot, function, getRegisterOffset(1, 2));
	// DMA Bus mastering, and memory space access are on.
	pciCommand |= 6;
	UINT16_T pciStatus = pciReadWordRegister(bus, slot, function, getRegisterOffset(1, 0));
	pciWriteDwordRegister(bus, slot, function, getRegisterOffset(1,0), pciStatus | pciCommand);

	for(UINTPTR_T base = 0x410000, virt = 0x410000, phys = baseAddr; virt < (base + hbaSize); virt += 4096, phys += 4096)
		MapPhysicalTo(phys, (PVOID)virt, ALLOC_FLAGS_CACHE_DISABLE | ALLOC_FLAGS_WRITE_ENABLED);

	g_abar = (PVOID)0x410000;
	const char* versionStr = "Unknown. Rather the driver is living in the past, or the nvme controller is broken.";
	BOOL leave = false;
	switch (g_abar->version)
	{
	case NVME_VERSION_1_0_0:
		versionStr = "1.0.0";
		break;
	case NVME_VERSION_1_1_0:
		versionStr = "1.1.0";
		break;
	case NVME_VERSION_1_2_0:
		versionStr = "1.2.0";
		break;
	case NVME_VERSION_1_2_1:
		versionStr = "1.2.1";
		break;
	case NVME_VERSION_1_3_0:
		versionStr = "1.3.0";
		break;
	case NVME_VERSION_1_4_0:
		versionStr = "1.4.0";
		break;
	case NVME_VERSION_2_0_0:
		versionStr = "2.0.0";
		break;
	default:
		leave = true;
		break;
	}

	Printf("NVME Log: Controller is nvme version %s.\r\n", versionStr);
	
	if (leave)
	{
		Printf("NVME Fatal Error: Unsupported nvme version for the controller.\r\n");
		return 1;
	}

	sti();

	return 0;
}

asm(".intel_syntax noprefix;"
	"VirtualAlloc: ;"
		"lea ebx, [esp+4];"
		"mov eax, 36;"
		"int 0x40;"
		"ret;"
);