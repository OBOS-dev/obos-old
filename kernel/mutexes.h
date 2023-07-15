/*
	mutexes.h

	Copyright (c) 2023 Omar Berrow
*/

#ifndef __OBOS_MUTEXES_H
#define __OBOS_MUTEXES_H
#include "types.h"

/// <summary>
/// Makes a new mutex.
/// </summary>
/// <returns>A handle to a new mutex. If the function fails, it returns OBOS_INVALID_HANDLE_VALUE. Call GetLastError for the error code.</returns>
HANDLE MakeMutex();
/// <summary>
/// Locks a mutex.
/// </summary>
/// <param name="hMutex">The mutex to lock.</param>
/// <returns>FALSE on failure, otherwise TRUE. If the function fails, call GetLastError for the error code.</returns>
BOOL LockMutex(HANDLE hMutex);
/// <summary>
/// Unlocks a mutex. If the current thread isn't the one that locked the mutex, the function will fail with OBOS_ERROR_ACCESS_DENIED.
/// </summary>
/// <param name="hMutex">The mutex to unlock.</param>
/// <returns>FALSE on failure, otherwise TRUE. If the function fails, call GetLastError for the error code.</returns>
BOOL UnlockMutex(HANDLE hMutex);
/// <summary>
/// Destroys a mutex.
/// </summary>
/// <param name="hMutex">The mutex to destroy.</param>
/// <returns>FALSE on failure, otherwise TRUE. If the function fails, call GetLastError for the error code.</returns>
BOOL DestroyMutex(HANDLE hMutex);
#endif