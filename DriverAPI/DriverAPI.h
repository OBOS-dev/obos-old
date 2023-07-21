/*
	DriverAPI.h

	Copyright (c) 2023 Omar Berrow
*/
// Used by drivers as a helper library.

#ifndef __OBOS_DRIVERAPI_H
#define __OBOS_DRIVERAPI_H

#include <types.h>
#include <multiboot.h>

#ifndef OBOS_DRIVER_ID
#error The driver id must be defined.
#endif

#define CallSyscall(syscallId, arguments, isDriverSyscall) _CallSyscall(OBOS_DRIVER_ID, syscallId, arguments, isDriverSyscall)
#define RegisterDriver() _RegisterDriver(OBOS_DRIVER_ID)

/// <summary>
/// No error. When OBOS_DriverEntry returns this, it succeeded.
/// This cannot cause a kernel panic.
/// </summary>
#define OBOS_DRIVER_NO_ERROR ((DWORD)0)
/// <summary>
/// The driver is not needed.
/// This cannot cause a kernel panic.
/// </summary>
#define OBOS_DRIVER_ERROR_NOT_NEEDED ((DWORD)1)
/// <summary>
/// A driver specific error. If the driver returns this, it is expected to set the error code in g_driverErrorCode.
/// This will cause a kernel panic.
/// </summary>
#define OBOS_DRIVER_SPECIFIC_ERROR ((DWORD)2)

/// <summary>
/// The driver syscall IDs.
/// </summary>
enum
{
	/// <summary>
	/// DWORD RegisterDriver(DWORD driverId, DWORD(*OBOS_DriverDetach)());
	/// Syscall: 0x00
	/// Registers a driver.
	/// </summary>
	/// <param name="driverId">The driver's id. This is stored in ecx.</param>
	/// <param name="OBOS_DriverDetach">The driver's exit point.</param>
	/// <returns>Zero on success, otherwise an error code.</returns>
	DRIVER_SYSCALL_REGISTER,
	/// <summary>
	/// void DriverLog(DWORD driverId, CSTRING message, SIZE_T size);
	/// Syscall: 0x01
	/// Registers a driver.
	/// </summary>
	/// <param name="driverId">The driver's id. This is stored in ecx.</param>
	/// <param name="message">The message to print.</param>
	/// <param name="size">The size of the message to print.</param>
	DRIVER_SYSCALL_LOG,
};

#define OBOS_DRIVER_SYSCALL_NO_ERROR ((DWORD)0)
#define OBOS_DRIVER_SYSCALL_ERROR_ALREADY_REGISTERED ((DWORD)1)

#ifdef __cplusplus
extern "C"
{
#endif

	/// <summary>
	/// The driver's entry point.
	/// </summary>
	/// <param name="header">The multiboot header.</param>
	/// <returns>An error code. OBOS_DRIVER_NO_ERROR on success, or OBOS_DRIVER_ERROR_* for an error.</returns>
	DWORD OBOS_DriverEntry(multiboot_info_t header);
	/// <summary>
	/// The driver's exit point. When the computer shuts down, or when the driver is no longer needed (the device was disconnected, for example), this is called.
	/// </summary>
	/// <returns>An error code. Anything other than zero is an error, and might cause a kernel panic.</returns>
	DWORD OBOS_DriverDetach();

	/// <summary>
	/// Calls a syscall.
	/// </summary>
	/// <param name="driverId">The driver's id</param>
	/// <param name="syscallId">The syscall's id</param>
	/// <param name="arguments">The arguments to the syscall.</param>
	/// <param name="isDriverSyscall">Whether to use int 0x40 (user mode interrupts) or int 0x50 (driver interrupts).</param>
	/// <returns>The syscall's return value.</returns>
	DWORD _CallSyscall(DWORD driverId, DWORD syscallId, PVOID arguments, BOOL isDriverSyscall);
	/// <summary>
	/// Registers the driver. This mainly changes the ring from ring 3 to ring 0.
	/// </summary>
	/// <param name="driverId">The driver's id.</param>
	/// <returns>The return code of the syscall.</returns>
	DWORD _RegisterDriver(DWORD driverId);
#ifdef __cplusplus
}
#endif

#endif