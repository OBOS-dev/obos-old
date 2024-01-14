/*
	drivers/generic/fat/cache.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <hashmap.h>
#include <vector.h>
#include <pair.h>

#define fat32FirstSectorOfCluster(cluster, bpb, first_data_sector) (uint32_t)((((cluster) - 2) * (bpb).sectorsPerCluster) + (first_data_sector))

namespace fatDriver
{
	struct cacheEntry
	{
		char* path = nullptr;
		uint8_t fileAttributes = 0; // See obos::driverInterface::fileAttributes
		size_t filesize = 0; // Shouldn't ever pass 0xffffffff because well, FAT32.
		obos::utils::Vector<uint64_t> clusters; // Must be in growing order.
		struct partition* owner = nullptr;
		cacheEntry *next, *prev;
	};
	enum class fatType
	{
		INVALID,
		FAT12 = 12,
		FAT16 = 16,
		FAT32 = 32,
	};
	struct partition
	{
		uint32_t driveId = 0;
		uint8_t  partitionId = 0;
		fatType fat_type = fatType::INVALID;
		cacheEntry *head = nullptr,
                   *tail = nullptr;
		::size_t nCacheEntries = 0;
		uint32_t FatSz = 0;
		uint32_t RootDirSectors = 0;
		uint32_t TotSec = 0;
		uint32_t DataSec = 0;
		uint32_t FirstDataSec = 0;
		uint32_t ClusterCount = 0;
		struct generic_bpb* bpb;
	};
	bool operator==(const partition& first, const partition& second);
	extern obos::utils::Vector<partition> g_partitions;
	typedef obos::utils::pair<uint32_t, uint8_t> partitionIdPair;
	extern obos::utils::Hashmap<partitionIdPair, ::size_t> g_partitionToIndex;

	void ProbeDrives();
}