/*
	drivers/generic/fat/main.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>

#include <multitasking/thread.h>

#include <driverInterface/struct.h>

#include <multitasking/threadAPI/thrHandle.h>

using namespace obos;

#ifdef __GNUC__
#define DEFINE_IN_SECTION __attribute__((section(OBOS_DRIVER_HEADER_SECTION_NAME)))
#else
#define DEFINE_IN_SECTION
#endif

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
				.QueryFileProperties = nullptr,
				.FileIteratorCreate = nullptr,
				.FileIteratorNext = nullptr,
				.FileIteratorClose = nullptr,
				.ReadFile = nullptr,
				.unused = { nullptr,nullptr,nullptr,nullptr }
			}
		}
	}
};

extern "C" void _start()
{
	
	g_driverHeader.driver_initialized = true;
	while (g_driverHeader.driver_finished_loading);
	thread::ExitThread(0);
}
