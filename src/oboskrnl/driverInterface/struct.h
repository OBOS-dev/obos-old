/*
	driverInterface/struct.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <stdarg.h>

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
			/*struct {
				bool vprintfFunctionRequest : 1;
				bool setStackSizeRequest : 1;
				bool initrdLocationRequest : 1;
			} requests;*/
			enum _requests
			{
				VPRINTF_FUNCTION_REQUEST = 1,
				SET_STACK_SIZE_REQUEST = 2,
				INITRD_LOCATION_REQUEST = 4,
				PANIC_FUNCTION_REQUEST = 8,
				MEMORY_MANIPULATION_FUNCTIONS_REQUEST = 16,
			};
			uint64_t requests;
			size_t stackSize; // SET_STACK_SIZE_REQUEST
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
			size_t(*vprintfFunctionResponse)(const char* format, va_list list); // VPRINTF_FUNCTION_REQUEST
			void(*panicFunctionResponse)(const char* format, va_list list); // PANIC_FUNCTION_REQUEST
			struct {
				void* addr;
				size_t size;
			} initrdLocationResponse; // INITRD_LOCATION_REQUEST
			struct {
				void* (*memzero)(void* block, size_t size); // fills block to block+size with zeros.
				void* (*memcpy)(void* dest, const void* src, size_t size); // copies size bytes of src to dest
				bool (*memcmp)(const void* blk1, const void* blk2, size_t size); // returns false when the blocks do not match, otherwise
				bool (*memcmp_toByte)(const void* blk1, uint32_t val, size_t size); // returns false if one of the bytes does not match cal
				size_t(*strlen)(const char* string); // counts the bytes in string until it gets to a null byte
				bool (*strcmp)(const char* str1, const char* str2); // checks if strlen(str1) == strlen(str2), and if it is, memcmp's str1 and str2.
			} memoryManipFunctionsResponse; // MEMORY_MANIPULATION_FUNCTIONS_REQUEST
		};
	}
}