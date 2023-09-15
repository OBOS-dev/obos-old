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

#ifdef __x86_64__
#define OBOS_API UINTPTR_T,UINTPTR_T,UINTPTR_T,UINTPTR_T,UINTPTR_T,UINTPTR_T,
#define PASS_OBOS_API_PARS 0,0,0,0,0,0,
#else
#define OBOS_API
#define PASS_OBOS_API_PARS
#endif

enum exitStatus RegisterDriver(OBOS_API DWORD driverID, enum serviceType type);
enum exitStatus RegisterInterruptHandler(OBOS_API DWORD driverId, BYTE interruptId, void(*handler)());
enum exitStatus PicSendEoi(OBOS_API BYTE irq);
enum exitStatus DisableIRQ(OBOS_API BYTE irq);
enum exitStatus EnableIRQ(OBOS_API BYTE irq);
enum exitStatus RegisterReadCallback(OBOS_API DWORD driverId, void(*callback)(STRING outputBuffer, SIZE_T sizeBuffer));
enum exitStatus PrintChar(OBOS_API CHAR ch, BOOL flush);
enum exitStatus GetMultibootModule(OBOS_API DWORD moduleIndex, UINTPTR_T* moduleStart, SIZE_T* size);
enum exitStatus RegisterFileReadCallback(OBOS_API DWORD driverId, void(*callback)(CSTRING filename, STRING outputBuffer, SIZE_T sizeBuffer));
/// <summary>
/// This documentation is for the callback.
/// Gets info about a file.
/// </summary>
/// <param name="filename">The file's name</param>
/// <param name="size">An output parameter. If the file exists, this should be set to the file's size.</param>
/// <returns>A bitfield. Look at driver_api/enums.h:enum fileExistsReturn for information on what this can be. If the file doesn't exist, return 0.</returns>
enum exitStatus RegisterFileExistsCallback(OBOS_API DWORD driverId, char(*callback)(CSTRING filename, SIZE_T* size));
enum exitStatus MapPhysicalTo(OBOS_API UINTPTR_T physicalAddress, PVOID virtualAddress, UINT32_T flags);
enum exitStatus UnmapPhysicalTo(OBOS_API PVOID virtualAddress);
enum exitStatus Printf(OBOS_API CSTRING format, ...);
enum exitStatus GetPhysicalAddress(OBOS_API PVOID linearAddress, PVOID* physicalAddress);
enum exitStatus RegisterRecursiveFileIterateCallback(OBOS_API DWORD driverId, void(*callback)(void(*appendEntry)(CSTRING filename, SIZE_T bufSize)));

enum exitStatus CallSyscall(OBOS_API DWORD syscallId, ...);

#ifdef __cplusplus
}
#endif