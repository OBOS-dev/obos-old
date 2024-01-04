/*
	drivers/generic/initrd/main.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>

#include <stdarg.h>

#include <multitasking/thread.h>

#include <driverInterface/struct.h>

#include <multitasking/threadAPI/thrHandle.h>

#include "parse.h"
#include "interface.h"

using namespace obos;

#ifdef __GNUC__
#define DEFINE_IN_SECTION __attribute__((section(OBOS_DRIVER_HEADER_SECTION_NAME)))
#else
#define DEFINE_IN_SECTION
#endif

driverInterface::driverHeader DEFINE_IN_SECTION g_driverHeader = {
	.magicNumber = obos::driverInterface::OBOS_DRIVER_HEADER_MAGIC,
	.driverId = 0,
	.driverType = obos::driverInterface::OBOS_SERVICE_TYPE_INITRD_FILESYSTEM,
	.requests = driverInterface::driverHeader::REQUEST_INITRD_LOCATION,
	.functionTable = {
		.GetServiceType = []()->driverInterface::serviceType { return driverInterface::serviceType::OBOS_SERVICE_TYPE_INITRD_FILESYSTEM; },
		.serviceSpecific = {
			.filesystem = {
				.QueryFileProperties = initrdInterface::QueryFileProperties,
				.FileIteratorCreate = initrdInterface::IterCreate,
				.FileIteratorNext = initrdInterface::IterNext,
				.FileIteratorClose = initrdInterface::IterClose,
				.ReadFile = initrdInterface::ReadFile,
				.unused = { nullptr,nullptr,nullptr,nullptr }
			}
		}
	}
};

extern void ConnectionHandler(uintptr_t);

#ifdef __GNUC__
#define WEAK __attribute__((weak))
#else
#define WEAK
#endif

extern "C" void _start()
{
	if (!g_driverHeader.initrdLocationResponse.addr)
		logger::panic(nullptr, "InitRD Driver: No initrd image received from the kernel.\n");
	logger::log("InitRD Driver: Initializing filesystem cache.\n");
	InitializeFilesystemCache();
	g_driverHeader.driver_initialized = true;
	while (g_driverHeader.driver_finished_loading);
	thread::ExitThread(0);
}
