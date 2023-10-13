/*
	oboskrnl/arch/x86_64/irq.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>

#include <arch/x86_64/interrupt.h>

#include <limine.h>

#include <x86_64-utils/asm.h>
#define packed __attribute__((packed))

namespace obos
{
	enum irqControllers
	{
		IRQ_CONTROLLER_PIC,
		IRQ_CONTROLLER_APIC
	} g_irqController;
	static volatile limine_rsdp_request rsdp_request = {
		.id = LIMINE_RSDP_REQUEST,
		.revision = 0,
	};
	struct ACPIRSDPHeader {
		char Signature[8];
		uint8_t Checksum;
		char OEMID[6];
		uint8_t Revision;
		uint32_t RsdtAddress; // Deprecated

		// Fields only valid if Revision != 0
		uint32_t Length;
		uint64_t XsdtAddress;
		uint8_t ExtendedChecksum;
		uint8_t reserved[3];
	} packed;
	struct ACPISDTHeader {
		char Signature[4];
		uint32_t Length;
		uint8_t Revision;
		uint8_t Checksum;
		char OEMID[6];
		char OEMTableID[8];
		uint32_t OEMRevision;
		uint32_t CreatorID;
		uint32_t CreatorRevision;
	} packed;
	
	void RemapPIC()
	{
		// The first pic.
		outb(0x20, 0x10 | 0x01);
		outb(0x21, 0x20);
		outb(0x21, 0x4);
		outb(0x21, 0x1);

		// The second pic.
		outb(0xA0, 0x10 | 0x01);
		outb(0xA1, 0x28);
		outb(0xA1, 0x2);
		outb(0xA1, 0x1);
	}
	struct MADTTable
	{
		ACPISDTHeader sdtHeader;
		uint32_t lapicAddress;
		uint32_t unwanted;
		// There are more entries.
	} packed;
	struct MADT_EntryHeader
	{
		uint8_t type;
		uint8_t length;
	} packed;
	struct MADT_EntryType0
	{
		MADT_EntryHeader entryHeader;
		uint8_t processorID;
		uint8_t apicID;
		uint32_t flags;
	} packed;
	struct MADT_EntryType1
	{
		MADT_EntryHeader entryHeader;
		uint8_t ioApicID;
		uint8_t resv1;
		uint32_t ioapicAddress;
		uint32_t globalSystemInterruptBase;
	} packed;
	struct MADT_EntryType2
	{
		MADT_EntryHeader entryHeader;
		uint8_t busSource;
		uint8_t irqSource;
		uint32_t globalSystemInterrupt;
		uint16_t flags;
	} packed;
	struct MADT_EntryType3
	{
		MADT_EntryHeader entryHeader;
		uint8_t nmiSource;
		uint8_t resv;
		uint16_t flags;
		uint32_t globalSystemInterrupt;
	} packed;
	struct MADT_EntryType4
	{
		MADT_EntryHeader entryHeader;
		uint8_t processorID;
		uint16_t flags;
		uint8_t lINT;
	} packed;
	struct MADT_EntryType5
	{
		MADT_EntryHeader entryHeader;
		uint8_t resv1[2];
		uintptr_t lapic_address;
	} packed;
	struct MADT_EntryType9
	{
		MADT_EntryHeader entryHeader;
		uint8_t resv1[2];
		uint32_t x2APIC_ID;
		uint32_t flags;
		uint32_t acpiID;
	} packed;
	size_t ProcessEntryHeader(MADT_EntryHeader* entryHeader, void*& lapicAddressOut, void*& ioapicAddressOut, uint8_t& nCores, uint8_t* processorIDs)
	{
		size_t ret = 0;
		switch (entryHeader->type)
		{
		case 0:
		{
			MADT_EntryType0* entry = (MADT_EntryType0*)entryHeader;
			processorIDs[nCores++] = entry->processorID;
			ret = sizeof(MADT_EntryType0);
			break;
		}
		case 1:
		{
			MADT_EntryType1* entry = (MADT_EntryType1*)entryHeader;
			ioapicAddressOut = (void*)(uintptr_t)entry->ioapicAddress;
			ret = sizeof(MADT_EntryType1);
			break;
		}
		case 2:
			ret = sizeof(MADT_EntryType2);
			break;
		case 3:
			ret = sizeof(MADT_EntryType3);
			break;
		case 4:
			ret = sizeof(MADT_EntryType4);
			break;
		case 5:
		{
			MADT_EntryType5* entry = (MADT_EntryType5*)entryHeader;
			lapicAddressOut = (void*)entry->lapic_address;
			ret = sizeof(MADT_EntryType5);
			break;
		}
		case 9:
			ret = sizeof(MADT_EntryType9);
			break;
		default:
			break;
		}
		return ret;
	}
	void InitializeAPIC(MADTTable* madtTable)
	{
		// Disable both PICs, and remap them, just in case a spurious interrupt happens.
		outb(0x21, 0xFF);
		outb(0xA1, 0xFF);
		RemapPIC();

		MADT_EntryHeader* entryHeader = reinterpret_cast<MADT_EntryHeader*>(madtTable + 1);
		void* end = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(madtTable) + madtTable->sdtHeader.Length);
		void* lapicAddress = (void*)(uintptr_t)madtTable->lapicAddress;
		void* ioapicAddress = nullptr;
		uint8_t processorIDs[256];
		uint8_t nCores = 0;
		for (; entryHeader != end; entryHeader = reinterpret_cast<MADT_EntryHeader*>(reinterpret_cast<uintptr_t>(entryHeader) + entryHeader->length))
			ProcessEntryHeader(entryHeader, lapicAddress, ioapicAddress, nCores, &processorIDs[0]);
		logger::log("Found %d cores, local apic address: %p, io apic address: %p, processor ids: \n", nCores, lapicAddress, ioapicAddress);
		uint32_t oldForeground = 0;
		uint32_t oldBackground = 0;
		g_kernelConsole.GetColour(&oldForeground, &oldBackground);
		g_kernelConsole.SetColour(logger::GREEN, oldBackground);
		for (uint8_t i = 0; i < nCores; i++)
			logger::printf("\t%p\n", processorIDs[i]);
		g_kernelConsole.SetColour(oldForeground, oldBackground); \
	}	
	void InitializeIrq()
	{
		g_irqController = IRQ_CONTROLLER_PIC;
		ACPIRSDPHeader* rsdp = (ACPIRSDPHeader*)rsdp_request.response->address;
		if (!rsdp)
		{
			RemapPIC();
			return;
		}
		if (!utils::memcmp(rsdp->Signature, "RSD PTR ", 8) || rsdp->Revision != 2)
		{
			logger::warning("%s: Falling back to the PIC.\n", __func__);
			RemapPIC(); // We must fall back to the PIC.
			return;
		}
		logger::log("%s: OEM %c%c%c%c%c\n", __func__, rsdp->OEMID[0], rsdp->OEMID[1], rsdp->OEMID[2], rsdp->OEMID[3], rsdp->OEMID[4]);
		ACPISDTHeader* xsdt = (ACPISDTHeader*)rsdp->XsdtAddress;
		ACPISDTHeader** tableAddresses = reinterpret_cast<ACPISDTHeader**>(xsdt + 1);
		MADTTable* madtTable;
		for (uint32_t i = 0; i < (xsdt->Length - sizeof(*xsdt)) / 8; i++)
		{
			ACPISDTHeader* currentSDT = tableAddresses[i];
			if (utils::memcmp(currentSDT->Signature, "APIC", 4))
			{
				madtTable = (MADTTable*)currentSDT;
				break;
			}
		}
		if (madtTable)
		{
			g_irqController = IRQ_CONTROLLER_APIC;
			logger::log("%s: Using the APIC as the irq controller.\n", __func__);
			InitializeAPIC(madtTable);
			return;
		}
		RemapPIC();
	}
}