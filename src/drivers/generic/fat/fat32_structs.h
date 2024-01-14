/*
	drivers/generic/fat/fat32_structs.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

namespace fatDriver
{
	struct ebpb_fat32
	{
		uint32_t sectorsPerFAT;
		uint16_t flags;
		uint8_t version[2]; // version[0]: Minor Version, version[1]: Major Version
		uint32_t rootDirectoryCluster;
		uint16_t fsInfoSector;
		uint16_t backupBootSector;
		byte resv1[12];
		uint8_t driveNumber;
		uint8_t resv2_or_ntFlags;
		uint8_t signature; // 0x28 or 0x29
		uint32_t volumeIDSerialNumber;
		char volumeLabel[11];
		char sysIdentifierString[8];
		uint8_t bootcode[420];
		uint16_t mbrSignature;
	} __attribute__((packed));
}