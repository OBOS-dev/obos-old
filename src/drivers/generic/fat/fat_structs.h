/*
	drivers/generic/fat/fat_structs.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include "fat32_structs.h"

namespace fatDriver
{
	struct generic_bpb
	{
		uint8_t jmp[3];
		char oem_identifer[8];
		uint16_t bytesPerSector;
		uint8_t sectorsPerCluster;
		uint16_t nResvSectors;
		uint8_t nFats;
		uint16_t nRootDirectoryEntries;
		uint16_t totalSectorCountOnVolume16; // If zero, use totalSectorCountOnVolume32
		uint8_t mediaDescriptorType;
		uint16_t sectorsPerFAT;
		uint16_t sectorsPerTrack;
		uint16_t nHeadsSidesInMedia;
		uint32_t nHiddenSectors;
		uint32_t totalSectorCountOnVolume32;
		union
		{
			ebpb_fat32 fat32_ebpb;
		} ebpb;
	} __attribute__((packed));
	struct fat_timestamp
	{
		byte second : 5; // Multiply value by two for the actual value.
		byte minute : 6;
		byte   hour : 5;
	} __attribute__((packed));
	struct fat_date
	{
		byte      day : 5;
		byte    month : 4;
		byte year1980 : 7;
	} __attribute__((packed));
	static_assert(sizeof(fat_date) == 2);
	static_assert(sizeof(fat_timestamp) == 2);
	struct fat_dirEntry
	{
		enum fat_dirEntryType
		{
			READ_ONLY = 0x01,
			HIDDEN    = 0x02,
			SYSTEM    = 0x04,
			VOLUME_ID = 0x08,
			DIRECTORY = 0x10,
			ARCHIVE   = 0x20,
			LFN       = READ_ONLY|HIDDEN|SYSTEM|VOLUME_ID,
		};
		char fname[11];
		byte fAttribs;
		byte ntResv;
		byte creationTimeHunderthsSec;
		fat_timestamp creationTime;
		fat_date creationDate;
		fat_date accessDate;
		uint16_t cluster16_31;
		fat_timestamp modifyTime;
		fat_date modifyDate;
		uint16_t cluster0_15;
		uint32_t filesize;
	} __attribute__((packed));
	struct fat_lfn
	{
		uint8_t order; // To get the order, and with ~0x40. If (order & 0x40), the current LFN is the last.
		char16_t name1_5[5];
		uint8_t attribs; // Must be fat_dirEntry::LFN
		uint8_t type; 
		uint8_t checksum;
		char16_t name6_11[6];
		uint16_t ignored;
		char16_t name12_13[2];
	} __attribute__((packed));
	static_assert(sizeof(generic_bpb) == 512, "sizeof(generic_bpb) isn't 512 bytes.");
	static_assert(sizeof(fat_dirEntry) == 32, "sizeof(fat_dirEntry) isn't 32 bytes.");
	static_assert(sizeof(fat_lfn) == 32, "sizeof(fat_dirEntry) isn't 32 bytes.");
}