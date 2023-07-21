// Freestanding Headers from the kernel.
#include <types.h>
#include <multiboot.h>

#ifdef __cplusplus
#error Must be compiled with a C compiler.
#endif

#define OBOS_DRIVER_ID 0
// The driver api.
#include <DriverAPI/DriverAPI.h>

// For the kernel.
DWORD g_driverErrorCode = 0;

DWORD OBOS_DriverEntry(multiboot_info_t header)
{
	// Register the driver.
	RegisterDriver();
	UINTPTR_T arguments[2] = {
		(UINTPTR_T)"Hello, driver space!\r\n",
		23
	};
	// Print the message.
	CallSyscall(1, (PVOID)arguments, TRUE);
	
	return 0;
}

DWORD OBOS_DriverDetach()
{
	return 0;
}