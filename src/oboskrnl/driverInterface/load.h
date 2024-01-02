/*
	driverInterface/load.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <multitasking/threadAPI/thrHandle.h>

namespace obos
{
	namespace driverInterface
	{
		extern struct driverIdentity** g_driverInterfaces;
		extern size_t g_driverInterfacesCapacity;
		void ScanAndLoadModules();
		bool LoadModule(byte* file, size_t size, thread::ThreadHandle** mainThread);
	}
}