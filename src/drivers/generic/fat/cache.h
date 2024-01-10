/*
	drivers/generic/fat/cache.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <klog.h>

namespace fatDriver
{
	struct cacheEntry
	{
		const char* path = nullptr;
		uint64_t lbaOffset = 0;
		uint32_t driveId = 0, partitionId = 0;
		
	};
	void CacheInitializeForDrive();
	void ProbeDrives();
}