/*
	oboskrnl/arch/x86_64/irq/acpi.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace acpi
	{
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
		} __attribute__((packed));
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
		} __attribute__((packed));
		struct MADTTable
		{
			ACPISDTHeader sdtHeader;
			uint32_t lapicAddress;
			uint32_t unwanted;
			// There are more entries.
		} __attribute__((packed));
		struct MADT_EntryHeader
		{
			uint8_t type;
			uint8_t length;
		} __attribute__((packed));
		struct MADT_EntryType0
		{
			MADT_EntryHeader entryHeader;
			uint8_t processorID;
			uint8_t apicID;
			uint32_t flags;
		} __attribute__((packed));
		struct MADT_EntryType1
		{
			MADT_EntryHeader entryHeader;
			uint8_t ioApicID;
			uint8_t resv1;
			uint32_t ioapicAddress;
			uint32_t globalSystemInterruptBase;
		} __attribute__((packed));
		struct MADT_EntryType2
		{
			MADT_EntryHeader entryHeader;
			uint8_t busSource;
			uint8_t irqSource;
			uint32_t globalSystemInterrupt;
			uint16_t flags;
		} __attribute__((packed));
		struct MADT_EntryType3
		{
			MADT_EntryHeader entryHeader;
			uint8_t nmiSource;
			uint8_t resv;
			uint16_t flags;
			uint32_t globalSystemInterrupt;
		} __attribute__((packed));
		struct MADT_EntryType4
		{
			MADT_EntryHeader entryHeader;
			uint8_t processorID;
			uint16_t flags;
			uint8_t lINT;
		} __attribute__((packed));
		struct MADT_EntryType5
		{
			MADT_EntryHeader entryHeader;
			uint8_t resv1[2];
			uintptr_t lapic_address;
		} __attribute__((packed));
		struct MADT_EntryType9
		{
			MADT_EntryHeader entryHeader;
			uint8_t resv1[2];
			uint32_t x2APIC_ID;
			uint32_t flags;
			uint32_t acpiID;
		} __attribute__((packed));

		struct HPET_Addr
		{
			uint8_t addressSpaceId;
			uint8_t registerBitWidth;
			uint8_t registerBitOffset;
			uint8_t resv;
			uintptr_t address;
		}  __attribute__((packed));
		struct HPET_Table
		{
			ACPISDTHeader sdtHeader;
			uint32_t eventTimerBlockID;
			HPET_Addr baseAddress;
			uint8_t hpetNumber;
			uint16_t mainCounterMinimum/*ClockTickPeriodicMode*/;
			uint8_t pageProtectionAndOEMAttrib;
		} __attribute__((packed));
	}
}