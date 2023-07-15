/*
	error.h

	Copyright (c) 2023 Omar Berrow
*/

#ifndef __OBOS_ERROR_H
#define __OBOS_ERROR_H

#include "types.h"

/// <summary>
/// An invalid handle value. If a function that creates a handle returns this, it most likely failed.
/// </summary>
#define OBOS_INVALID_HANDLE_VALUE ((HANDLE)-1)

/// <summary>
/// No error has occurred.
/// </summary>
#define OBOS_NO_ERROR ((DWORD)0)
/// <summary>
/// An invalid handle value was given, or the api was not initialized.
/// </summary>
#define OBOS_ERROR_INVALID_HANDLE ((DWORD)1)
/// <summary>
/// No more memory.
/// </summary>
#define OBOS_ERROR_NO_MEMORY ((DWORD)2)
/// <summary>
/// Access was denied.
/// </summary>
#define OBOS_ERROR_ACCESS_DENIED ((DWORD)3)
/// <summary>
/// The thread did not terminate.
/// <.summary>
#define OBOS_ERROR_THREAD_NOT_DEAD ((DWORD)4)
/// <summary>
/// The thread was terminated.
/// </summary>
#define OBOS_ERROR_THREAD_DEAD ((DWORD)5)
/// <summary>
/// DestroyThread was called on the current thread.
/// </summary>
#define OBOS_ERROR_CANNOT_DESTROY_SELF ((DWORD)6)
/// <summary>
/// An invalid parameter was passed.
/// </summary>
#define OBOS_ERROR_INVALID_PARAMETER ((DWORD)7)
/// <summary>
///	The file was not found.
/// </summary>
#define OBOS_ERROR_FILE_NOT_FOUND ((DWORD)8)

/// <summary>
/// Gets the last error.
/// </summary>
/// <returns>The last error</returns>
DWORD GetLastError();
/// <summary>
/// Sets the last error.
/// </summary>
/// <param name="errCode">The error code.</param>
void SetLastError(DWORD errCode);

#endif