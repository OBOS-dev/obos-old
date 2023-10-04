/*
	oboskrnl/driver_api/load/module.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#include <driver_api/syscalls.h>

#include <multitasking/threadHandle.h>

namespace obos
{
	namespace driverAPI
	{
		driverIdentification* LoadModule(PBYTE file, SIZE_T size, multitasking::ThreadHandle* mainThreadHandle);
	}
}