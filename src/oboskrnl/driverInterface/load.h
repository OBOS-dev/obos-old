/*
	driverInterface/load.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <hashmap.h>

#include <multitasking/threadAPI/thrHandle.h>

namespace obos
{
	namespace driverInterface
	{
		extern utils::Hashmap<uint32_t, struct driverIdentity*> g_driverInterfaces;
		void ScanAndLoadModules(const char* root);
		// Returns the driver header, this header must be in an offset from file to file+size.
		struct driverHeader* CheckModule(byte* file, size_t size);
		bool LoadModule(byte* file, size_t size, thread::ThreadHandle** mainThread);
	}
}