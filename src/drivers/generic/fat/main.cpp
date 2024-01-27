/*
	drivers/generic/fat/main.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>

#include <multitasking/thread.h>

#include <driverInterface/struct.h>

#include <multitasking/threadAPI/thrHandle.h>

#include "cache.h"

using namespace obos;

#ifdef __GNUC__
#define DEFINE_IN_SECTION __attribute__((section(OBOS_DRIVER_HEADER_SECTION_NAME)))
#else
#define DEFINE_IN_SECTION
#endif

namespace fatDriver
{
	bool QueryFileProperties(
		const char* path,
		uint32_t driveId, uint32_t partitionIdOnDrive,
		size_t* oFsizeBytes,
		driverInterface::fileAttributes* oFAttribs);
	bool FileIteratorCreate(
		uint32_t driveId, uint32_t partitionIdOnDrive,
		uintptr_t* oIter);
	bool FileIteratorNext(
		uintptr_t iter,
		const char** oFilepath,
		void(**freeFunction)(void* buf),
		size_t* oFsizeBytes,
		driverInterface::fileAttributes* oFAttribs);
	bool FileIteratorClose(uintptr_t iter);
	bool ReadFile(
		uint32_t driveId, uint32_t partitionIdOnDrive,
		const char* path,
		size_t nToSkip,
		size_t nToRead,
		char* buff);
}

driverInterface::driverHeader DEFINE_IN_SECTION g_driverHeader = {
	.magicNumber = obos::driverInterface::OBOS_DRIVER_HEADER_MAGIC,
	.driverId = 4,
	.driverType = obos::driverInterface::OBOS_SERVICE_TYPE_FILESYSTEM,
	.requests = driverInterface::driverHeader::REQUEST_SET_STACK_SIZE,
	.stackSize = 0x10000,
	.functionTable = {
		.GetServiceType = []()->driverInterface::serviceType { return driverInterface::serviceType::OBOS_SERVICE_TYPE_FILESYSTEM; },
		.serviceSpecific = {
			.filesystem = {
				.QueryFileProperties = fatDriver::QueryFileProperties,
				.FileIteratorCreate = fatDriver::FileIteratorCreate,
				.FileIteratorNext = fatDriver::FileIteratorNext,
				.FileIteratorClose = fatDriver::FileIteratorClose,
				.ReadFile = fatDriver::ReadFile,
				.unused = { nullptr,nullptr,nullptr,nullptr }
			}
		}
	}
};

extern "C" void _start()
{
	logger::log("FAT Driver: Probing drives for FAT partitions.\n");
	fatDriver::ProbeDrives();
	g_driverHeader.driver_initialized = true;
	while (!g_driverHeader.driver_finished_loading);
	thread::ExitThread(0);
}
