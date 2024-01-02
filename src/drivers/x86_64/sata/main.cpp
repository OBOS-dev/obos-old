/*
	drivers/x86_64/sata/main.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>

#include <driverInterface/struct.h>
#include <driverInterface/register_drive.h>

#include <multitasking/threadAPI/thrHandle.h>

#include <driverInterface/x86_64/enumerate_pci.h>

#include <arch/x86_64/memory_manager/virtual/initialize.h>

#include <arch/x86_64/memory_manager/physical/allocate.h>

#include <allocators/vmm/arch.h>
#include <allocators/liballoc.h>

#include <x86_64-utils/asm.h>

#include "structs.h"
#include "command.h"

using namespace obos;

namespace obos
{
	namespace memory
	{
		OBOS_EXPORT void* MapPhysicalAddress(PageMap* pageMap, uintptr_t phys, void* to, uintptr_t cpuFlags);
	}
}

#ifdef __GNUC__
#define DEFINE_IN_SECTION __attribute__((section(OBOS_DRIVER_HEADER_SECTION_NAME)))
#else
#define DEFINE_IN_SECTION
#endif

extern bool DriveReadSectors(
	uint32_t driveId,
	uint64_t lbaOffset,
	size_t nSectorsToRead,
	void** buff,
	size_t* oNSectorsRead
	);
extern bool DriveWriteSectors(
	uint32_t driveId,
	uint64_t lbaOffset,
	size_t nSectorsToWrite,
	char* buff,
	size_t* oNSectorsWrote
	);
extern bool DriveQueryInfo(
	uint32_t driveId,
	uint64_t *oNSectors,
	uint64_t *oBytesPerSector
	);

driverInterface::driverHeader DEFINE_IN_SECTION g_driverHeader = {
	.magicNumber = obos::driverInterface::OBOS_DRIVER_HEADER_MAGIC,
	.driverId = 0,
	.driverType = obos::driverInterface::OBOS_SERVICE_TYPE_STORAGE_DEVICE,
	.requests = driverInterface::driverHeader::REQUEST_SET_STACK_SIZE,
    .stackSize = 0x8000,
	.functionTable = {
		.GetServiceType = []()->driverInterface::serviceType { return driverInterface::serviceType::OBOS_SERVICE_TYPE_STORAGE_DEVICE; },
		.serviceSpecific = {
			.storageDevice = {
				.ReadSectors = DriveReadSectors,
				.WriteSectors = DriveWriteSectors,
				.QueryDiskInfo = DriveQueryInfo,
				.unused = {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,}
			}
		}
	},
    .howToIdentifyDevice = 0b1, // PCI
    .pciInfo = {
        .classCode = 0x1, // Class code 0x01
        .subclass = 1<<6, // Subclass: 0x06
        .progIf = 1<<1 // Prog IF: 0x01
    }
};

volatile byte* g_hbaBase = nullptr;
volatile HBA_MEM* g_generalHostControl = nullptr;
Port g_ports[32];

void InitializeAHCI(uint32_t*, uint8_t bus, uint8_t slot, uint8_t function)
{
	// Map the HBA memory registers.

	uint16_t pciCommand = driverInterface::pciReadWordRegister(bus, slot, function, PCI_GetRegisterOffset(1, 2));
	// DMA Bus mastering, and memory space access are on.
	pciCommand |= 6;
	uint16_t pciStatus = driverInterface::pciReadWordRegister(bus, slot, function, PCI_GetRegisterOffset(1, 0));
	driverInterface::pciWriteDwordRegister(bus, slot, function, PCI_GetRegisterOffset(1, 0), pciStatus | pciCommand);

	uintptr_t baseAddr = driverInterface::pciReadDwordRegister(bus, slot, function, PCI_GetRegisterOffset(9, 0)) & (~0b1111);

	driverInterface::pciWriteDwordRegister(bus, slot, function, PCI_GetRegisterOffset(9, 0), 0xFFFFFFFF);
	size_t hbaSize = (~driverInterface::pciReadDwordRegister(bus, slot, function, PCI_GetRegisterOffset(9, 0)) & (~0b1111)) + 1;
	hbaSize = ((hbaSize >> 12) + 1) << 12;
	driverInterface::pciWriteDwordRegister(bus, slot, function, PCI_GetRegisterOffset(9, 0), baseAddr);

	size_t hbaSizePages = hbaSize / 0x1000;
	g_hbaBase = (byte*)memory::_Impl_FindUsableAddress(nullptr, hbaSizePages);
	g_generalHostControl = (HBA_MEM*)g_hbaBase;

	for (size_t i = 0; i < hbaSizePages; i++)
	{
		memory::MapPhysicalAddress(
			memory::getCurrentPageMap(),
			baseAddr + i * 0x1000,
			(void*)(g_hbaBase + i * 0x1000),
			(1<<0)|(1<<1)|(1<<4)|(((uintptr_t)1)<<63) /*PRESENT,WRITE,CACHE_DISABLE,EXECUTE_DISABLE*/
			);
		}

	// Initialize the controller.

	// Take ownership of the controller, if supported.
	if (g_generalHostControl->cap2 & (1<<0))
	{
		g_generalHostControl->bohc |= (1<<1); // OOS Bit
		while (g_generalHostControl->bohc & (1<<0) /*BOS Bit*/);
	}
	// We're AHCI aware.
	g_generalHostControl->ghc.ae = true;

	// Initialize all ports.

	for (byte port = 0; port < 32; port++)
	{
		if (!(g_generalHostControl->pi & (1<<port)))
			continue;
		volatile HBA_PORT* pPort = g_generalHostControl->ports + port;

		uint32_t ssts = pPort->ssts;

		uint8_t ipm = (ssts >> 8) & 0x0F;
		uint8_t det = ssts & 0x0F;

		if (det != HBA_PORT_DET_PRESENT || ipm != HBA_PORT_IPM_ACTIVE)
		{
			logger::warning("AHCI: No drive found on port %d, even though it is implemented.\n");
			continue;
		}

		if (pPort->cmd & (1<<0) /*PxCMD.ST*/)
		{
			pPort->cmd &= ~(1<<0);
			while(pPort->cmd & (1<<15) /*PxCMD.CR*/);
		}
		if (pPort->cmd & (1<<4) /*PxCMD.FRE*/)
		{
			pPort->cmd &= ~(1<<4);
			while(pPort->cmd & (1<<14) /*PxCMD.FR*/);
		}
		void* realClFisBase = memory::_Impl_FindUsableAddress(nullptr, 3);
		uintptr_t clBase = memory::allocatePhysicalPage(2);
		utils::memzero(memory::mapPageTable((uintptr_t*)clBase), 4096);
		utils::memzero(memory::mapPageTable((uintptr_t*)(clBase + 4096)), 4096);
		pPort->clb = clBase & 0xffffffff;
		if (g_generalHostControl->cap.s64a)
			pPort->clbu = clBase & ~(0xffffffff);
		else
			if (clBase & ~(0xffffffff))
				logger::panic(nullptr, "AHCI: clBase has its upper 32-bits set and cap.s64a is false.\n");
		
		uintptr_t fisBase = memory::allocatePhysicalPage();
		utils::memzero(memory::mapPageTable((uintptr_t*)fisBase), 4096);
		pPort->fb = fisBase & 0xffffffff;
		if (g_generalHostControl->cap.s64a)
			pPort->fbu = fisBase & ~(0xffffffff);
		else
			if (fisBase & ~(0xffffffff))
				logger::panic(nullptr, "AHCI: fisBase has its upper 32-bits set and cap.s64a is false.\n");
		auto& portDescriptor = g_ports[port];
		portDescriptor.id = port;
		portDescriptor.clBase = memory::MapPhysicalAddress(
			memory::getCurrentPageMap(),
			clBase,
			realClFisBase,
			(1<<0)|(1<<1)|(1<<4)|(((uintptr_t)1)<<63) /*PRESENT,WRITE,CACHE_DISABLE,EXECUTE_DISABLE*/
		);
		memory::MapPhysicalAddress(
			memory::getCurrentPageMap(),
			clBase + 0x1000,
			(byte*)realClFisBase + 0x1000,
			(1<<0)|(1<<1)|(1<<4)|(((uintptr_t)1)<<63) /*PRESENT,WRITE,CACHE_DISABLE,EXECUTE_DISABLE*/
		);
		portDescriptor.fisBase = memory::MapPhysicalAddress(
			memory::getCurrentPageMap(),
			fisBase,
			(byte*)realClFisBase + 0x2000,
			(1<<0)|(1<<1)|(1<<4)|(((uintptr_t)1)<<63) /*PRESENT,WRITE,CACHE_DISABLE,EXECUTE_DISABLE*/
		);
		portDescriptor.clBasePhys = clBase;
		portDescriptor.fisBasePhys = fisBase;
		portDescriptor.hbaPort = pPort;
		pPort->serr = 0xFFFFFFFF;
		for (uint8_t slot = 0; slot < 32; slot++)
		{
			HBA_CMD_HEADER* cmdHeader = (HBA_CMD_HEADER*)portDescriptor.clBase + slot;
			uintptr_t ctba = clBase + sizeof(HBA_CMD_HEADER) * 32 + slot * sizeof(HBA_CMD_TBL);
			cmdHeader->ctba = (uint32_t)ctba & 0xffffffff;
			if (g_generalHostControl->cap.s64a)
				cmdHeader->ctbau = ctba & ~(0xffffffff);
			else
				if (ctba & ~(0xffffffff))
					logger::panic(nullptr, "AHCI: %s: ctba has its upper 32-bits set and cap.s64a is false.\n", __func__);
		}
		// Send IDENTIFIY ATA to find out information about the connected drives (eg: sector count, sector size)
		// The "command engine" is already stopped.
		// StopCommandEngine(pPort);
		uint32_t cmdSlot = FindCMDSlot(pPort);
		memory::VirtualAllocator vallocator{ nullptr };
		uintptr_t responsePhys = 0;
		volatile byte* response = (volatile byte*)vallocator.VirtualAlloc(nullptr, 4096, memory::PROT_DISABLE_CACHE | memory::PROT_NO_COW_ON_ALLOCATE);
		responsePhys = (uintptr_t)memory::getCurrentPageMap()->getL1PageMapEntryAt((uintptr_t)response) & 0xFFFFFFFFFF000;
		// Setup the command slot.
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
		command->command = ATA_IDENTIFY_DEVICE;
		command->device = 0x40;
		command->c = 1;
		// Wait for the port.
		// 0x88: ATA_DEV_BUSY | ATA_DEV_DRQ
		while ((pPort->tfd & 0x88))
			pause();
		// Issue the command
		pPort->ci = (1 << cmdSlot);
		// Wait for the command.
		// TODO: Make this interrupt driven.
		StartCommandEngine(pPort);
		while ( (pPort->ci & (1 << cmdSlot)) && !(pPort->is & (0xFD800000)) /*An error in the IS.*/ );
		portDescriptor.sectorSize = *(uint32_t*)(response + (117 * 2));
		if (!portDescriptor.sectorSize)
			portDescriptor.sectorSize = 512; // Assume one sector = 512 bytes.
		portDescriptor.nSectors = *(uint64_t*)(response + (100 * 2));
		portDescriptor.driveType = pPort->sig == SATA_SIG_ATA ? Port::DRIVE_TYPE_SATA : Port::DRIVE_TYPE_SATAPI;
		vallocator.VirtualFree((void*)response, 4096);
		// Register the drive with the kernel.
		portDescriptor.kernelID = driverInterface::RegisterDrive();
		logger::info("AHCI: Found %s drive at port %d. Kernel drive ID: %d, sector count: %e0x%X, sector size %e0x%X.\n",
			portDescriptor.driveType == Port::DRIVE_TYPE_SATA ? "SATA" : "SATAPI",
			port,
			portDescriptor.kernelID,
			16,
			portDescriptor.nSectors,
			8,
			portDescriptor.sectorSize);
	}
}

extern "C" void _start()
{
	uint32_t exitCode = 0;
	uint8_t bus = 0, slot = 0, function = 0;
	if (!driverInterface::enumeratePci(0x01, 0x06, 0x01, &slot, &function, &bus))
	{
		exitCode = 1;
		goto done;
	}
	logger::log("AHCI: Initializing AHCI.\n");
	InitializeAHCI(&exitCode, bus, slot, function);
	if (!exitCode)
		logger::log("AHCI: Initialized AHCI.\n");
	else
		logger::error("AHCI: Could not initialize AHCI.\n");
	{
		void* buff = nullptr;
		DriveReadSectors(0, 0, 1025, &buff, nullptr);
		memory::VirtualAllocator{nullptr}.VirtualFree(buff, 1025 * 4096);
	}
	done:
    g_driverHeader.driver_initialized = true;
	while (g_driverHeader.driver_finished_loading);
	thread::ExitThread(exitCode);
}