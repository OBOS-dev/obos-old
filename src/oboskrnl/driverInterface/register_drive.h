/*
    oboskrnl/driverInterface/register_drive.h

    Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

namespace obos
{
	namespace driverInterface
	{
		/// <summary>
		/// Registers a drive in the kernel. A disk driver must call this for every drive that it recognizes.
		/// </summary>
		/// <returns>The drive's id in the kernel.</returns>
		OBOS_EXPORT uint32_t RegisterDrive();
		/// <summary>
		/// Unregisters a drive in the kernel.
		/// </summary>
		/// <param name="id">The drive's id in the kernel. This is obtained through RegisterDrive()</param>
		OBOS_EXPORT void UnregisterDrive(uint32_t id);
	}
}