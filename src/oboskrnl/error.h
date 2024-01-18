/*
	oboskrnl/error.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

namespace obos
{
	enum obos_error
	{
		/// <summary>
		/// No error.
		/// </summary>
		OBOS_SUCCESS,
		/// <summary>
		/// No such object could be found.
		/// </summary>
		OBOS_ERROR_NO_SUCH_OBJECT,
		/// <summary>
		/// An invalid parameter was given to a function.
		/// </summary>
		OBOS_ERROR_INVALID_PARAMETER,
		/// <summary>
		/// The handle was already opened.
		/// </summary>
		OBOS_ERROR_ALREADY_EXISTS,
		/// <summary>
		/// The handle was unopened.
		/// </summary>
		OBOS_ERROR_UNOPENED_HANDLE,
		/// <summary>
		/// The thread is dead.
		/// </summary>
		OBOS_ERROR_THREAD_DIED,
		/// <summary>
		/// The base address that is trying to be allocated is in use.
		/// </summary>
		OBOS_ERROR_BASE_ADDRESS_USED,
		/// <summary>
		/// The program loader is trying to load a program that isn't the architecture that the kernel was built for.
		/// </summary>
		OBOS_ERROR_LOADER_INCORRECT_ARCHITECTURE,
		/// <summary>
		/// The program loader was provided with an incorrect file.
		/// </summary>
		OBOS_ERROR_LOADER_INCORRECT_FILE,
		/// <summary>
		/// The program that was trying to be loaded as a driver lacked the driver header, or a connection with DriverClient was being made to a process that isn't a driver.
		/// </summary>
		OBOS_ERROR_NOT_A_DRIVER,
		/// <summary>
		/// The mutex was locked.
		/// </summary>
		OBOS_ERROR_MUTEX_LOCKED,
		/// <summary>
		/// The scheduler woke up the thread during a blocking operation when g_timerTicks > thread->wakeUpTime.
		/// </summary>
		OBOS_ERROR_TIMEOUT,
		/// <summary>
		/// The connection was closed.
		/// </summary>
		OBOS_ERROR_CONNECTION_CLOSED,
		/// <summary>
		/// Access has been denied.
		/// </summary>
		OBOS_ERROR_ACCESS_DENIED,
		/// <summary>
		/// The block wasn't mapped.
		/// </summary>
		OBOS_ERROR_ADDRESS_UNMAPPED,
		/// <summary>
		/// A free region was not found while allocating.
		/// </summary>
		OBOS_ERROR_NO_FREE_REGION,
		/// <summary>
		/// There is already a mount point.
		/// </summary>
		OBOS_ERROR_VFS_ALREADY_MOUNTED,
		/// <summary>
		/// The partition is already mounted.
		/// </summary>
		OBOS_ERROR_VFS_PARTITION_ALREADY_MOUNTED,
		/// <summary>
		/// The driver for a mount point does not report to be a filesystem driver.
		/// </summary>
		OBOS_ERROR_VFS_NOT_A_FILESYSTEM_DRIVER,
		/// <summary>
		/// The file was not found.
		/// </summary>
		OBOS_ERROR_VFS_FILE_NOT_FOUND,
		/// <summary>
		/// The read was aborted.
		/// </summary>
		OBOS_ERROR_VFS_READ_ABORTED,
		/// <summary>
		/// The feature hasn't been implemented (yet).
		/// </summary>
		OBOS_ERROR_UNIMPLEMENTED_FEATURE,
		/// <summary>
		/// A page fault occurred.
		/// </summary>
		OBOS_ERROR_PAGE_FAULT,
		/// <summary>
		/// The driver's code referenced an undefined symbol in the kernel.
		/// </summary>
		OBOS_ERROR_DRIVER_REFERENCED_UNRESOLVED_SYMBOL,
		/// <summary>
		/// The driver's code referenced a variable that had a different size in the kernel.
		/// </summary>
		OBOS_ERROR_DRIVER_SYMBOL_MISMATCH,
		/// <summary>
		/// The driver has an invalid or corrupt header.
		/// </summary>
		OBOS_ERROR_INVALID_DRIVER_HEADER,
		/// <summary>
		/// A driver function returned false.
		/// </summary>
		OBOS_ERROR_VFS_DRIVER_FAILURE,
		/// <summary>
		/// The signal handler is nullptr.
		/// </summary>
		OBOS_ERROR_NULL_HANDLER,
		/// <summary>
		/// The write was aborted.
		/// </summary>
		OBOS_ERROR_VFS_WRITE_ABORTED,
		/// <summary>
		/// The DriveHandle doesn't refer to a partition.
		/// </summary>
		OBOS_ERROR_VFS_HANDLE_NOT_REFERRING_TO_PARTITION,
		/// <summary>
		/// The object was read only and Write* was called on it.
		/// </summary>
		OBOS_ERROR_VFS_READ_ONLY,
		/// <summary>
		/// A mount on a partition with an unrecognized filesystem was made.
		/// </summary>
		OBOS_ERROR_VFS_UNRECOGNIZED_PARTITION_FS,

		OBOS_ERROR_HIGHEST_VALUE,
	};
	OBOS_EXPORT void SetLastError(uint32_t err);
	OBOS_EXPORT uint32_t GetLastError();
}