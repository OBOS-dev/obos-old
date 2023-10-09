/*
	driver_api/syscall_interface.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#include <driver_api/enums.h>

#ifdef __cplusplus
extern "C" {
#define OBOS_SET_DEFAULT_PARAMETER(v) = v
#define OBOS_EXIT_STATUS obos::driverAPI::exitStatus
#define OBOS_ENUM obos::driverAPI::
#else
#define SET_DEFAULT_PARAMETER(v) = v
#define OBOS_EXIT_STATUS enum exitStatus
#define OBOS_ENUM enum
#endif

#ifdef __x86_64__
#define OBOS_API UINTPTR_T,UINTPTR_T,UINTPTR_T,UINTPTR_T,UINTPTR_T,UINTPTR_T,
#define PASS_OBOS_API_PARS 0,0,0,0,0,0,
#else
#define OBOS_API
#define PASS_OBOS_API_PARS
#endif

OBOS_EXIT_STATUS RegisterInterruptHandler(OBOS_API BYTE interruptId, void(*handler)());
OBOS_EXIT_STATUS PicSendEoi(OBOS_API BYTE irq);
OBOS_EXIT_STATUS DisableIRQ(OBOS_API BYTE irq);
OBOS_EXIT_STATUS EnableIRQ(OBOS_API BYTE irq);
OBOS_EXIT_STATUS PrintChar(OBOS_API CHAR ch, BOOL flush);
OBOS_EXIT_STATUS GetMultibootModule(OBOS_API DWORD moduleIndex, UINTPTR_T* moduleStart, SIZE_T* size);
OBOS_EXIT_STATUS MapPhysicalTo(OBOS_API UINTPTR_T physicalAddress, PVOID virtualAddress, UINTPTR_T flags);
OBOS_EXIT_STATUS UnmapPhysicalTo(OBOS_API PVOID virtualAddress);
OBOS_EXIT_STATUS Printf(OBOS_API CSTRING format, ...);
OBOS_EXIT_STATUS GetPhysicalAddress(OBOS_API PVOID linearAddress, PVOID* physicalAddress);
OBOS_EXIT_STATUS ListenForConnections(OBOS_API HANDLE* newHandle);
OBOS_EXIT_STATUS ConnectionSendData(OBOS_API HANDLE handle, PVOID data, SIZE_T size, BOOL failIfMutexLocked OBOS_SET_DEFAULT_PARAMETER(true));
OBOS_EXIT_STATUS ConnectionRecvData(OBOS_API HANDLE handle, PVOID data, SIZE_T size, BOOL waitForData OBOS_SET_DEFAULT_PARAMETER(true), BOOL peek OBOS_SET_DEFAULT_PARAMETER(false), BOOL failIfMutexLocked OBOS_SET_DEFAULT_PARAMETER(true));
OBOS_EXIT_STATUS ConnectionClose(OBOS_API HANDLE handle);
OBOS_EXIT_STATUS HeapAllocate(OBOS_API PVOID* ret, SIZE_T size);
OBOS_EXIT_STATUS HeapFree(OBOS_API PVOID block);

OBOS_EXIT_STATUS CallSyscall(OBOS_API DWORD syscallId, ...);

#ifdef __cplusplus
}
#endif