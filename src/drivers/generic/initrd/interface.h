/*
	drivers/generic/initrd/interface.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <driverInterface/struct.h>

namespace initrdInterface
{
	bool QueryFileProperties (
		const char* path,
		uint64_t driveId, uint8_t partitionIdOnDrive,
		size_t* oFsizeBytes,
		uint64_t* oLBAOffset,
		obos::driverInterface::fileAttributes* oFAttribs);
	bool IterCreate(
		uint64_t driveId, uint8_t partitionIdOnDrive,
		uintptr_t* oIter);
	bool IterNext(
		uintptr_t iter,
		const char** oFilepath,
		void(**freeFunction)(void* buf),
		size_t* oFsizeBytes,
		uint64_t* oLBAOffset,
		obos::driverInterface::fileAttributes* oFAttribs);
	bool IterClose(uintptr_t iter);
	bool ReadFile(
		uint64_t driveId, uint8_t partitionIdOnDrive,
		const char* path,
		size_t nToSkip,
		size_t nToRead,
		char* buff);
}