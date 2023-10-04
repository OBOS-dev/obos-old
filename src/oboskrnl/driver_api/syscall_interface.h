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
enum exitStatus RegisterInterruptHandler(OBOS_API BYTE interruptId, void(*handler)());
enum exitStatus PicSendEoi(OBOS_API BYTE irq);
enum exitStatus DisableIRQ(OBOS_API BYTE irq);
enum exitStatus EnableIRQ(OBOS_API BYTE irq);
enum exitStatus PrintChar(OBOS_API CHAR ch, BOOL flush);
enum exitStatus GetMultibootModule(OBOS_API DWORD moduleIndex, UINTPTR_T* moduleStart, SIZE_T* size);
enum exitStatus MapPhysicalTo(OBOS_API UINTPTR_T physicalAddress, PVOID virtualAddress, UINT32_T flags);
enum exitStatus UnmapPhysicalTo(OBOS_API PVOID virtualAddress);
enum exitStatus Printf(OBOS_API CSTRING format, ...);
enum exitStatus GetPhysicalAddress(OBOS_API PVOID linearAddress, PVOID* physicalAddress);

enum exitStatus CallSyscall(OBOS_API DWORD syscallId, ...);

#ifdef __cplusplus
}
#endif