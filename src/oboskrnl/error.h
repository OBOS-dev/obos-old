/*
	oboskrnl/error.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

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
		/// The block was already freed.
		/// </summary>
		OBOS_ERROR_ADDRESS_ALREADY_FREED,
		/// <summary>
		/// A free region was not found while allocated.
		/// </summary>
		OBOS_ERROR_NO_FREE_REGION,
		/// <summary>
		/// There is already a mount point.
		/// </summary>
		OBOS_ERROR_VFS_ALREADY_MOUNTED,
		/// <summary>
		/// 
		/// </summary>
		OBOS_ERROR_VFS_PARTITION_ALREADY_MOUNTED,
		OBOS_ERROR_VFS_NOT_A_FILESYSTEM_DRIVER,
		OBOS_ERROR_VFS_FILE_NOT_FOUND,
		OBOS_ERROR_VFS_READ_ABORTED,
		OBOS_ERROR_UNIMPLEMENTED_FEATURE,

		OBOS_ERROR_HIGHEST_VALUE,
	};
#ifdef OBOS_KERNEL
	void SetLastError(uint32_t err);
	uint32_t GetLastError();
#endif
}