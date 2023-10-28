/*
	driverInterface/struct.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <driverInterface/interface.h>

#include <multitasking/threadAPI/thrHandle.h>

#define OBOS_DRIVER_HEADER_SECTION_NAME ".obosDriverHeader"

namespace obos
{
	namespace driverInterface
	{
		enum { OBOS_DRIVER_HEADER_MAGIC = 0x5902E288 };
		enum serviceType
		{
			OBOS_SERVICE_TYPE_FILESYSTEM,
			OBOS_SERVICE_TYPE_STORAGE_DEVICE, OBOS_SERVICE_TYPE_VIRTUAL_STORAGE_DEVICE,
			OBOS_SERVICE_TYPE_USER_INPUT_DEVICE, OBOS_SERVICE_TYPE_VIRTUAL_USER_INPUT_DEVICE,
			OBOS_SERVICE_TYPE_COMMUNICATION, OBOS_SERVICE_TYPE_VIRTUAL_COMMUNICATION,
		};
		enum fileAttributes
		{
			FILE_DOESNT_EXIST,
			// If this is set, the file cannot be written to.
			FILE_ATTRIBUTES_READ_ONLY = 1,
			// If this is set, the file links to another file.
			FILE_ATTRIBUTES_SYMLINK = 2,
			// If this is set, this is a directory.
			FILE_ATTRIBUTES_DIRECTORY = 4,
			// If this is set, the file links to another file.
			FILE_ATTRIBUTES_HARDLINK = 8,
			// If this is set, this is a file, not a directory.
			FILE_ATTRIBUTES_FILE = 16,
		};
		struct driverIdentity
		{
			uint32_t driverId;
			uint32_t _serviceType;
			DriverConnectionList connections;
			bool listening;
			void* process;
		};
		struct bitfield128
		{
			uint64_t bytes0_7;
			uint64_t bytes7_15;
		};
		struct driverHeader
		{
			uint32_t magicNumber;
			uint32_t driverId;
			uint32_t driverType;
			size_t stackSize;
			/// <summary>
			/// Bit 0: PCI
			/// </summary>
			uint32_t howToIdentifyDevice;
			struct __driverInfoPciInfo
			{
				uint32_t classCode;
				/// <summary>
				/// If a bit is set, the bit number will be the value.
				/// <para></para>
				/// This bitfield can have more than bit set (for multiple values).
				/// </summary>
				struct bitfield128 subclass;
				struct bitfield128 progIf;
			} pciInfo;
		};
	}
}