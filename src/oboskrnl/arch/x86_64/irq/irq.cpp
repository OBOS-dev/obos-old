/*
	oboskrnl/arch/x86_64/irq/irq.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>

#include <arch/x86_64/interrupt.h>

#include <arch/x86_64/irq/acpi.h>
#include <arch/x86_64/irq/irq.h>

#include <limine.h>

#include <x86_64-utils/asm.h>

#define IA32_APIC_BASE 0x1b

namespace obos
{
	static volatile limine_rsdp_request rsdp_request = {
		.id = LIMINE_RSDP_REQUEST,
		.revision = 0,
	};
	
	void RemapPIC()
	{
		// The first pic.
		outb(0x20, 0x11);
		outb(0x21, 0x20);
		outb(0x21, 0x4);
		outb(0x21, 0x1);

		// The second pic.
		outb(0xA0, 0x11);
		outb(0xA1, 0x28);
		outb(0xA1, 0x2);
		outb(0xA1, 0x1);
	}
	
	volatile LAPIC* g_localAPICAddr = nullptr;

	size_t ProcessEntryHeader(acpi::MADT_EntryHeader* entryHeader, void*& lapicAddressOut, void*& ioapicAddressOut, uint8_t& nCores, uint8_t* processorIDs)
	{
		size_t ret = 0;
		switch (entryHeader->type)
		{
		case 0:
		{
			acpi::MADT_EntryType0* entry = (acpi::MADT_EntryType0*)entryHeader;
			processorIDs[nCores++] = entry->processorID;
			ret = sizeof(acpi::MADT_EntryType0);
			break;
		}
		case 1:
		{
			acpi::MADT_EntryType1* entry = (acpi::MADT_EntryType1*)entryHeader;
			ioapicAddressOut = (void*)(uintptr_t)entry->ioapicAddress;
			ret = sizeof(acpi::MADT_EntryType1);
			break;
		}
		case 2:
			ret = sizeof(acpi::MADT_EntryType2);
			break;
		case 3:
			ret = sizeof(acpi::MADT_EntryType3);
			break;
		case 4:
			ret = sizeof(acpi::MADT_EntryType4);
			break;
		case 5:
		{
			acpi::MADT_EntryType5* entry = (acpi::MADT_EntryType5*)entryHeader;
			lapicAddressOut = (void*)entry->lapic_address;
			ret = sizeof(acpi::MADT_EntryType5);
			break;
		}
		case 9:
			ret = sizeof(acpi::MADT_EntryType9);
			break;
		default:
			break;
		}
		return ret;
	}
	void DefaultInterruptHandler(interrupt_frame* frame)
	{
		if(frame->intNumber != 0xff)
			SendEOI();
	}
	void InitializeAPIC(acpi::MADTTable* madtTable)
	{
		// Disable both PICs, and remap them, just in case a spurious interrupt happens.
		outb(0x21, 0xFF);
		outb(0xA1, 0xFF);
		RemapPIC();

		acpi::MADT_EntryHeader* entryHeader = reinterpret_cast<acpi::MADT_EntryHeader*>(madtTable + 1);
		void* end = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(madtTable) + madtTable->sdtHeader.Length);
		void* lapicAddress = (void*)(uintptr_t)madtTable->lapicAddress;
		void* ioapicAddress = nullptr;
		uint8_t processorIDs[256];
		uint8_t nCores = 0;
		for (; entryHeader != end; entryHeader = reinterpret_cast<acpi::MADT_EntryHeader*>(reinterpret_cast<uintptr_t>(entryHeader) + entryHeader->length))
			ProcessEntryHeader(entryHeader, lapicAddress, ioapicAddress, nCores, &processorIDs[0]);
		logger::log("Found %d cores, local apic address: %p, io apic address: %p.\n", nCores, lapicAddress, ioapicAddress);
		g_localAPICAddr = (LAPIC*)lapicAddress;
		/*logger::log("Dumping LAPIC registers.\n" 
					"\tlapicID: %p, lapicVersion: %p.\n"
					"\ttaskPriority: %p, processorPriority: %p.\n"
					"\tremoteRead: %p, logicalDestination: %p.\n"
					"\tdestinationFormat: %p, spuriousInterruptVector: %p.\n"
					"\terrorStatus: %p, lvtCMCI: %p.\n"
					"\tinterruptCommand0_31: %p, interruptCommand32_63: %p.\n"
					"\tlvtTimer: %p, lvtThermalSensor: %p.\n"
					"\tlvtPerformanceMonitoringCounters: %p, lvtLINT0: %p.\n"
					"\tlvtLINT1: %p, lvtError: %p.\n"
					"\tinitialCount: %p, currentCount: %p.\n"
					"\tdivideConfig: %p\n",
			g_localAPICAddr->lapicID, g_localAPICAddr->lapicVersion, 
			g_localAPICAddr->taskPriority, g_localAPICAddr->processorPriority,
			g_localAPICAddr->remoteRead, g_localAPICAddr->logicalDestination,
			g_localAPICAddr->destinationFormat, g_localAPICAddr->spuriousInterruptVector,
			g_localAPICAddr->errorStatus, g_localAPICAddr->lvtCMCI,
			g_localAPICAddr->interruptCommand0_31, g_localAPICAddr->interruptCommand32_63,
			g_localAPICAddr->lvtTimer, g_localAPICAddr->lvtThermalSensor,
			g_localAPICAddr->lvtPerformanceMonitoringCounters, g_localAPICAddr->lvtLINT0,
			g_localAPICAddr->lvtLINT1, g_localAPICAddr->lvtError,
			g_localAPICAddr->initialCount, g_localAPICAddr->currentCount,
			g_localAPICAddr->divideConfig
		);*/
		uint64_t msr = rdmsr(IA32_APIC_BASE);
		msr |= ((uint64_t)1 << 11) | ((uint64_t)1 << 8);
		wrmsr(IA32_APIC_BASE, msr);

		g_localAPICAddr->errorStatus = 0;

		g_localAPICAddr->lvtLINT0 |= 0xf8;
		g_localAPICAddr->lvtLINT1 |= 0xf9;
		g_localAPICAddr->lvtError = 0xfa;
		g_localAPICAddr->lvtCMCI = 0xfb;
		g_localAPICAddr->lvtPerformanceMonitoringCounters = 0xfc;
		g_localAPICAddr->lvtThermalSensor = 0xfd;
		g_localAPICAddr->lvtTimer = 0xfe;
		g_localAPICAddr->spuriousInterruptVector = 0xff;

		for(byte i = 0xf8; i > 0; i++)
			RegisterInterruptHandler(i, DefaultInterruptHandler);
		g_localAPICAddr->spuriousInterruptVector |= (1 << 8);
		logger::log("%s: Initialized the APIC.\n", __func__);
	}
	void SendEOI()
	{
		g_localAPICAddr->eoi = 0;
	}
	void InitializeIrq()
	{
		acpi::ACPIRSDPHeader* rsdp = (acpi::ACPIRSDPHeader*)rsdp_request.response->address;
		if (!rsdp)
		{
			logger::panic("No RSDP table from bootloader.\n");
			RemapPIC();
			return;
		}
		logger::log("%s: System OEM: %c%c%c%c%c\n", __func__, rsdp->OEMID[0], rsdp->OEMID[1], rsdp->OEMID[2], rsdp->OEMID[3], rsdp->OEMID[4]);
		acpi::ACPISDTHeader* xsdt = (acpi::ACPISDTHeader*)rsdp->XsdtAddress;
		acpi::ACPISDTHeader** tableAddresses = reinterpret_cast<acpi::ACPISDTHeader**>(xsdt + 1);
		acpi::MADTTable* madtTable = nullptr;
		for (uint32_t i = 0; i < (xsdt->Length - sizeof(*xsdt)) / 8; i++)
		{
			acpi::ACPISDTHeader* currentSDT = tableAddresses[i];
			if (utils::memcmp(currentSDT->Signature, "APIC", 4))
			{
				madtTable = (acpi::MADTTable*)currentSDT;
				break;
			}
		}
		if (madtTable)
		{
			logger::log("%s: Using the APIC as the irq controller.\n", __func__);
			InitializeAPIC(madtTable);
			return;
		}
		logger::panic("No MADT Table found in the acpi tables.\n");
	}
}