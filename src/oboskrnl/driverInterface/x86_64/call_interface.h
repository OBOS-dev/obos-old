#pragma once

#include <int.h>

#include <driverInterface/interface.h>

#include <stdarg.h>

namespace obos
{
	size_t						   VPrintf(const char* str, va_list list);
	void						   ExitThread(uint32_t exitCode);
	driverInterface::DriverServer* AllocDriverServer();
	void						   FreeDriverServer(driverInterface::DriverServer* obj);
	void*						   Malloc(size_t size);
	void						   Free(void* blk);
	void*						   MapPhysToVirt(void* virt, void* phys, uintptr_t protFlags);
	void*						   GetInitrdLocation(size_t* size);
}