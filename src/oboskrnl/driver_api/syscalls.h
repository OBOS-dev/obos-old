/*
	driver_api/syscalls.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>
#include <driver_api/enums.h>

namespace obos
{
	namespace driverAPI
	{
		struct driverIdentification
		{
			DWORD driverId = 0;
			serviceType service_type = serviceType::SERVICE_TYPE_INVALID;
			PVOID readCallback = nullptr;
			PVOID existsCallback = nullptr;
		};
		extern driverIdentification** g_registeredDrivers;
		extern SIZE_T g_registeredDriversCapacity;
		void RegisterSyscalls();
	}
}