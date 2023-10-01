/*
	driver_api/enums.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#ifdef __cplusplus
#define DEFINE_ENUM enum class
#else
#define DEFINE_ENUM enum
#endif

#ifdef __cplusplus

namespace obos
{
	namespace driverAPI
	{

#endif
		DEFINE_ENUM serviceType
		{
			// Ex: A driver that hasn't been registered yet.
			OBOS_SERVICE_TYPE_INVALID = -1,
			// Ex: fat driver, ext2 driver.
			OBOS_SERVICE_TYPE_FILESYSTEM,
			// The ustarFilesystem driver.
			OBOS_SERVICE_TYPE_INITRD_FILESYSTEM,
			// Ex: SATA driver.
			OBOS_SERVICE_TYPE_STORAGE_DEVICE,
			OBOS_SERVICE_TYPE_VIRTUAL_STORAGE_DEVICE,
			// Ex: PS/2 Keyboard driver, PS/2 Mouse driver.
			OBOS_SERVICE_TYPE_USER_INPUT_DEVICE,
			// Ex: On-screen keyboard.
			OBOS_SERVICE_TYPE_VIRTUAL_USER_INPUT_DEVICE,
			// Ex: GPU driver, VGA driver.
			OBOS_SERVICE_TYPE_GRAPHICS_DEVICE,
			// Ex: A background process that shuts down the computer when the cpu temperature is too high.
			OBOS_SERVICE_TYPE_MONITOR,
			// Ex: A driver that allows pe files to be run.
			OBOS_SERVICE_TYPE_KERNEL_EXTENSION,
			// Ex: A serial driver, a network card driver.
			OBOS_SERVICE_TYPE_COMMUNICATION,
			// Ex: A virtual serial driver (see com0com for context).
			OBOS_SERVICE_TYPE_VIRTUAL_COMMUNICATION,
			// Not a service, instead it indicates the service type with the biggest value.
			OBOS_SERVICE_TYPE_MAX_VALUE = OBOS_SERVICE_TYPE_VIRTUAL_COMMUNICATION
		};
		DEFINE_ENUM exitStatus
		{
			EXIT_STATUS_INVALID_SYSCALL = -1,
			EXIT_STATUS_SUCCESS,
			EXIT_STATUS_ALREADY_REGISTERED,
			EXIT_STATUS_ACCESS_DENIED,
			EXIT_STATUS_NO_SUCH_DRIVER,
			EXIT_STATUS_INVALID_PARAMETER,
			EXIT_STATUS_ADDRESS_NOT_AVAILABLE,
			EXIT_STATUS_NOT_IMPLEMENTED
		};

		enum fileExistsReturn
		{
			// If this is set, the file cannot be written to.
			FILE_EXISTS_READ_ONLY = 1,
			// If this is set, the file links to another file.
			FILE_EXISTS_SYMLINK = 2,
			// If this is set, this is a directory.
			FILE_EXISTS_DIRECTORY = 4,
			// If this is set, the file links to another file.
			FILE_EXISTS_HARDLINK = 8,
			// If this is set, this is a file, not a directory.
			FILE_EXISTS_FILE = 16,
		};

		DEFINE_ENUM driver_commands
		{
			// Common Commands
			OBOS_SERVICE_GET_SERVICE_TYPE,
			// OBOS_SERVICE_TYPE_FILESYSTEM
			OBOS_SERVICE_QUERY_FILE_DATA,
			OBOS_SERVICE_MAKE_FILE_ITERATOR,
			OBOS_SERVICE_NEXT_FILE,
			// OBOS_SERVICE_TYPE_INITRD_FILESYSTEM <- OBOS_SERVICE_TYPE_FILESYSTEM
			OBOS_SERVICE_READ_FILE,
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

		enum allocFlags
		{
			ALLOC_FLAGS_WRITE_ENABLED = 2,
			ALLOC_FLAGS_GLOBAL = 4,
			ALLOC_FLAGS_CACHE_DISABLE = 16
		};


#ifdef __cplusplus
	}
}

#endif