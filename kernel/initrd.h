/*
	initrd.h

	Copyright (c) 2023 Omar Berrow
*/
#ifndef __OBOS_INITRD_H
#define __OBOS_INITRD_H

#include "types.h"

/// <summary>
/// Initializes the initial ramdisk. Not available outside of the kernel.
/// </summary>
/// <param name="startAddress">The start address. You usually get this from a module.</param>
/// <returns>An error code (OBOS_INITRD_*), or 0 on success.</returns>
VOID InitializeInitRD(PVOID startAddress);
/// <summary>
/// Reads a file from the initial ramdisk.
/// </summary>
/// <param name="filePath">The file path.</param>
/// <param name="bytesToRead">The amount of bytes to read.</param>
/// <param name="bytesRead">The amount of bytes read.</param>
/// <param name="output">The output buffer. If this parameter is NULLPTR, it is ignored and there is no output.</param>
/// <param name="offset">The file offset.</param>
/// <returns>TRUE on success, FALSE on failure. Use GetLastError for the error code.</returns>
BOOL ReadInitRDFile(CSTRING filePath, SIZE_T bytesToRead, SIZE_T* bytesRead, STRING output, SIZE_T offset);
/// <summary>
/// Gets the filesize of the file on the initial ramdisk.
/// </summary>
/// <param name="filePath">The file path.</param>
/// <param name="filesize">The file size.</param>
/// <returns>TRUE on success, FALSE on failure. Use GetLastError for the error code.</returns>
BOOL GetInitRDFileSize(CSTRING filePath, SIZE_T* filesize);

#endif