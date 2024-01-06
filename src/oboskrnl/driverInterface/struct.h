/*
	driverInterface/struct.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <stdarg.h>

#include <int.h>

#include <multitasking/threadAPI/thrHandle.h>

#define OBOS_DRIVER_HEADER_SECTION_NAME ".obosDriverHeader"

namespace obos
{
	namespace driverInterface
	{
		enum { OBOS_DRIVER_HEADER_MAGIC = 0x5902E288 };
		enum serviceType
		{
			OBOS_SERVICE_TYPE_FILESYSTEM, OBOS_SERVICE_TYPE_INITRD_FILESYSTEM,
			OBOS_SERVICE_TYPE_STORAGE_DEVICE, OBOS_SERVICE_TYPE_VIRTUAL_STORAGE_DEVICE,
			OBOS_SERVICE_TYPE_USER_INPUT_DEVICE, OBOS_SERVICE_TYPE_VIRTUAL_USER_INPUT_DEVICE,
			OBOS_SERVICE_TYPE_COMMUNICATION, OBOS_SERVICE_TYPE_VIRTUAL_COMMUNICATION,
		};
		enum driverCommands
		{
			OBOS_SERVICE_INVALID_SERVICE_COMMAND = -1,
			// Common Commands
			OBOS_SERVICE_GET_SERVICE_TYPE,
			// OBOS_SERVICE_TYPE_FILESYSTEM
			OBOS_SERVICE_QUERY_FILE_DATA,
			OBOS_SERVICE_MAKE_FILE_ITERATOR,
			OBOS_SERVICE_NEXT_FILE,
			OBOS_SERVICE_CLOSE_FILE_ITERATOR,
			OBOS_SERVICE_READ_FILE,
			// OBOS_SERVICE_TYPE_INITRD_FILESYSTEM <- OBOS_SERVICE_TYPE_FILESYSTEM
			// No commands yet.
			// OBOS_SERVICE_TYPE_STORAGE_DEVICE, SERVICE_TYPE_VIRTUAL_STORAGE_DEVICE
			OBOS_SERVICE_READ_LBA,
			OBOS_SERVICE_WRITE_LBA,
			// OBOS_SERVICE_TYPE_USER_INPUT_DEVICE, SERVICE_TYPE_VIRTUAL_USER_INPUT_DEVICE
			OBOS_SERVICE_READ_CHARACTER,
			// TODO: SERVICE_TYPE_GRAPHICS_DEVICE, SERVICE_TYPE_MONITOR, SERVICE_TYPE_KERNEL_EXTENSION
			// OBOS_SERVICE_TYPE_COMMUNICATION, OBOS_SERVICE_TYPE_VIRTUAL_COMMUNICATION
			OBOS_SERVICE_CONFIGURE_COMMUNICATION,
			OBOS_SERVICE_RECV_BYTE_FROM_DEVICE,
			OBOS_SERVICE_SEND_BYTE_TO_DEVICE,
		};
		enum fileAttributes
		{
			FILE_DOESNT_EXIST,
			// If this is set, the file cannot be written to.
			FILE_ATTRIBUTES_READ_ONLY = 1,
			// If this is set, the filesystem messed up.
			FILE_ATTRIBUTES_RESERVED = 2,
			// If this is set, this is a directory.
			FILE_ATTRIBUTES_DIRECTORY = 4,
			// If this is set, the file links to another file.
			FILE_ATTRIBUTES_HARDLINK = 8,
			// If this is set, this is a file, not a directory.
			FILE_ATTRIBUTES_FILE = 16,
		};
		struct ftable
		{
			static constexpr size_t maxCallbacks = 10 - 1; // The max - GetServiceType
			serviceType(*GetServiceType)();
			union
			{
				struct
				{
					/// <summary>
					/// Queries the file's properties.
					/// </summary>
					/// <param name="path">The path of the file to query.</param>
					/// <param name="driveId">The drive id the file is located on.</param>
					/// <param name="partitionIdOnDrive">The partition id on the drive the file is located on.</param>
					/// <param name="oFsizeBytes">The size of the file in bytes.</param>
					/// <param name="oLBAOffset">The lba offset of the file.</param>
					/// <param name="oFAttribs">The file properties</param>
					/// <returns>The function's status.</returns>
					bool(*QueryFileProperties)(
						const char* path,
						uint32_t driveId, uint8_t partitionIdOnDrive,
						size_t* oFsizeBytes,
						uint64_t* oLBAOffset,
						fileAttributes* oFAttribs);
					/// <summary>
					/// Creates a file iterator.
					/// </summary>
					/// <param name="driveId">The drive id the file is located on.</param>
					/// <param name="partitionIdOnDrive">The partition id on the drive the file is located on.</param>
					/// <param name="oIter">The variable to store the iterator in.</param>
					bool(*FileIteratorCreate)(
						uint32_t driveId, uint8_t partitionIdOnDrive,
						uintptr_t* oIter);
					bool(*FileIteratorNext)(
						uintptr_t iter,
						const char** oFilepath,
						void(**freeFunction)(void* buf),
						size_t* oFsizeBytes,
						uint64_t* oLBAOffset,
						fileAttributes* oFAttribs);
					bool(*FileIteratorClose)(uintptr_t iter);
					bool(*ReadFile)(
						uint32_t driveId, uint8_t partitionIdOnDrive,
						const char* path,
						size_t nToSkip,
						size_t nToRead,
						char* buff);
					// TODO: Make a write file
					void* unused[maxCallbacks - 5]; // Add padding
				} filesystem;
				struct
				{
					// If lbaOffset + nSectorsToRead > the drive's sector count, this function shall return true and set *oNSectorsRead to zero.
					// This function should use the kernel's VMM+PMM to allocate, not any heap, as it's freed with
					// VirtualFree().
					bool(*ReadSectors)(
						uint32_t driveId,
						uint64_t lbaOffset,
						size_t nSectorsToRead,
						void** buff,
						size_t* oNSectorsRead
						);
					// If lbaOffset + nSectorsToWrite > the drive's sector count, this function shall return true and set *oNSectorsWrote to zero.
					bool(*WriteSectors)(
						uint32_t driveId,
						uint64_t lbaOffset,
						size_t nSectorsToWrite,
						char* buff,
						size_t* oNSectorsWrote
						);
					// TODO: Find other useful information a program could use for a disk.
					bool(*QueryDiskInfo)(
						uint32_t driveId,
						uint64_t *oNSectors,
						uint64_t *oBytesPerSector
						);
					void* unused[maxCallbacks - 3]; // Add padding
				} storageDevice;
				struct
				{
					bool(*ReadCharacter)(
						uint64_t deviceId,
						char* ch
						);
					void* unused[maxCallbacks - 1]; // Add padding
				} userInputDevice;
				struct
				{
					bool(*InitializeConnection)(
						uint64_t deviceId,
						void* driverSpecificParameters,
						size_t szParameters
						);
					bool(*ReadCharacter)(
						uint64_t deviceId,
						char* ch
						);
					bool(*WriteCharacter)(
						uint64_t deviceId,
						char ch
						);
					void* unused[maxCallbacks - 3]; // Add padding
				} externalCommunication; // eg: NIC driver.
			} serviceSpecific;
		};
		struct driverIdentity
		{
			uint32_t driverId;
			uint32_t _serviceType;
			ftable functionTable;
			struct driverHeader* header;
		};
		struct driverHeader
		{
			uint32_t magicNumber;
			uint32_t driverId;
			uint32_t driverType;
			enum _requests
			{
				REQUEST_INITRD_LOCATION = 1,
				REQUEST_SET_STACK_SIZE = 2,
			};
			uint64_t requests;
			size_t stackSize; // SET_STACK_SIZE_REQUEST
			ftable functionTable;
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
				__uint128_t subclass;
				/// <summary>
				/// If a bit is set, the bit number will be the value.
				/// <para></para>
				/// This bitfield can have more than bit set (for multiple values).
				/// <para></para>
				/// If no bit is set any prog if is assumed.
				/// </summary>
				__uint128_t progIf;
			};
			__driverInfoPciInfo pciInfo;
			struct
			{
				void* addr;
				size_t size;
			} initrdLocationResponse;
			// Must be set by the driver when it's done initializing. The driver should not use the driver identity until 
			// driver_finished_loading is set by the kernel
			bool driver_initialized, driver_finished_loading;
		};
	}
}