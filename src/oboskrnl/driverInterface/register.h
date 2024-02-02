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
		enum class DeviceType
		{
			Drive,
			UserInput
		};
		/// <summary>
		/// Registers a device in the kernel.
		/// </summary>
		/// <param name="type">The type of the device.</param>
		/// <returns>The device id, or 0xffffffff on failure.</returns>
		OBOS_EXPORT uint32_t RegisterDevice(DeviceType type);
		/// <summary>
		/// Unregisters a device in the kernel.
		/// </summary>
		/// <param name="id">The device to unregister. This must be obtained through RegisterDevice.</param>
		/// <param name="type">The device type. This cannot be 0xffffffff.</param>
		OBOS_EXPORT void UnregisterDevice(uint32_t id, DeviceType type);
		/// <summary>
		/// Writes a byte to the input device's buffer.<para></para>
		/// This function does not SetLastError on failure.
		/// </summary>
		/// <param name="id">The device's id.</param>
		/// <param name="exChar">The character to write.</param>
		/// <returns>Whether the function succeeded (true) or not (false).</returns>
		OBOS_EXPORT bool WriteByteToInputDeviceBuffer(uint32_t id, uint16_t exChar);
		void* GetUserInputDevice(uint32_t id);
	}
}