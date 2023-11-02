/*
	drivers/generic/initrd/main.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>

#include <stdarg.h>

#include <multitasking/thread.h>

#include <driverInterface/struct.h>
#include <driverInterface/x86_64/call_interface.h>

#include <arch/x86_64/syscall/syscall_interface.h>

#include "parse.h"

using namespace obos;

driverInterface::driverHeader __attribute__((section(OBOS_DRIVER_HEADER_SECTION_NAME))) g_driverHeader = {
	.magicNumber = obos::driverInterface::OBOS_DRIVER_HEADER_MAGIC,
	.driverId = 0,
	.driverType = obos::driverInterface::OBOS_SERVICE_TYPE_INITRD_FILESYSTEM,
	.requests = driverInterface::driverHeader::VPRINTF_FUNCTION_REQUEST | 
				driverInterface::driverHeader::INITRD_LOCATION_REQUEST | 
				driverInterface::driverHeader::MEMORY_MANIPULATION_FUNCTIONS_REQUEST |
				driverInterface::driverHeader::PANIC_FUNCTION_REQUEST,
};

size_t printf(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	size_t ret = g_driverHeader.vprintfFunctionResponse(format, list);
	va_end(list);
	return ret;
}
[[noreturn]] void kpanic(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	g_driverHeader.panicFunctionResponse(format, list);
	va_end(list); // shouldn't get hit.
	while (1);
}

extern void ConnectionHandler(uintptr_t);

extern "C" void _start()
{
	if (!g_driverHeader.initrdLocationResponse.addr)
		kpanic("[DRIVER 0, FATAL] No initrd image received from the kernel.\n");
	printf("[DRIVER 0, LOG] Initializing filesystem cache.\n");
	InitializeFilesystemCache();
	// Listen for connections.
	driverInterface::DriverServer* currentClient = nullptr;
	while (1)
	{
		currentClient = AllocDriverServer();
		if (!currentClient->Listen())
		{
			printf("[DRIVER 0, ERROR] Listen: Error code: %d.\nTrying again...\n", g_driverHeader.driverId, GetLastError());
			FreeDriverServer(currentClient);
			continue;
		}
		uintptr_t thread = MakeThreadObject();
		if (!CreateThread(thread, thread::THREAD_PRIORITY_NORMAL, 0x4000, ConnectionHandler, (uintptr_t)currentClient, false))
		{
			printf("[DRIVER 0, ERROR] CreateThread: Error code: %d.\n", g_driverHeader.driverId, GetLastError());
			currentClient->CloseConnection();
			FreeDriverServer(currentClient);
			CloseThread(thread);
			FreeThread(thread);
			continue;
		}
		CloseThread(thread);
		FreeThread(thread);
	}
	ExitThread(0);
}