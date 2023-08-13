/*
	driver_api/syscall_interface.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#include <driver_api/enums.h>

#ifdef __cplusplus
extern "C" {
#endif

enum exitStatus RegisterDriver(DWORD driverID, enum serviceType type);
enum exitStatus RegisterInterruptHandler(DWORD driverId, BYTE interruptId, void(*handler)(const struct interrupt_frame* frame));
enum exitStatus PicSendEoi(BYTE irq);
enum exitStatus DisableIRQ(BYTE irq);
enum exitStatus EnableIRQ(BYTE irq);
enum exitStatus RegisterReadCallback(DWORD driverId, void(*callback)(STRING outputBuffer, SIZE_T sizeBuffer));
enum exitStatus PrintChar(CHAR ch, BOOL flush);
enum exitStatus GetMultibootModule(DWORD moduleIndex, UINTPTR_T* moduleStart, SIZE_T* size);
enum exitStatus RegisterFileReadCallback(DWORD driverId, void(*callback)(CSTRING filename, STRING outputBuffer, SIZE_T sizeBuffer));
/// <summary>
/// Gets info about a file.
/// This documentation is for the callback.
/// </summary>
/// <param name="filename">The file's name</param>
/// <param name="size">An output parameter. If the file exists, this should be set to the file's size.</param>
/// <returns>A bitfield. Look at driver_api/enums.h:enum fileExistsReturn for information on what this can be. If the file doesn't exist, return 0.</returns>
enum exitStatus RegisterFileExistsCallback(DWORD driverId, char(*callback)(CSTRING filename, SIZE_T* size));
enum exitStatus MapPhysicalTo(UINTPTR_T physicalAddress, PVOID virtualAddress, UINT32_T flags);
enum exitStatus UnmapPhysicalTo(PVOID virtualAddress);
enum exitStatus Printf(CSTRING format, ...);

enum exitStatus CallSyscall(DWORD syscallId, ...);

#ifdef __cplusplus
}
#endif