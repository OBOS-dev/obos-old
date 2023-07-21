#include "types.h"
#include "hashTable.h"
#include "terminal.h"
#include "klog.h"

#include "liballoc/liballoc_1_1.h"

hash_table* drivers;

static int hash(struct key* this)
{
	const char* str = (CSTRING)this->key;
	int hash = 5381;
	char c;

	for (int i = 0; i < sizeof(DWORD); i++, c = *str++)
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

	return hash;
}
static int compare(struct key* right, struct key* left)
{
	DWORD* a = (DWORD*)right->key;
	DWORD* b = (DWORD*)left->key;
	if (*a < *b)
		return -1;
	else if (*a > *b)
		return 1;
	return 0;
}

enum
{
	/// <summary>
	/// DWORD RegisterDriver(DWORD driverId, DWORD(*OBOS_DriverDetach)());
	/// Syscall: 0x00
	/// Registers a driver.
	/// </summary>
	/// <param name="driverId">The driver's id.</param>
	/// <param name="OBOS_DriverDetach">The driver's exit point.</param>
	/// <returns>Zero on success, otherwise an error code.</returns>
	DRIVER_SYSCALL_REGISTER,
	/// <summary>
	/// void RegisterDriver(DWORD driverId, CSTRING message, SIZE_T size);
	/// Syscall: 0x01
	/// Registers a driver.
	/// </summary>
	/// <param name="driverId">The driver's id.</param>
	/// <param name="message">The message to print.</param>
	/// <param name="size">The size of the message to print.</param>
	DRIVER_SYSCALL_LOG,
};

#define OBOS_DRIVER_NO_ERROR ((DWORD)0)
#define OBOS_DRIVER_ERROR_ALREADY_REGISTERED ((DWORD)1)

UINTPTR_T driver_syscall_handler(DWORD id, DWORD driverId, PVOID arguments)
{
	UINTPTR_T ret;
	switch (id)
	{
	case DRIVER_SYSCALL_REGISTER:
		if (!drivers)
			drivers = hash_table_create(256, hash, compare);
		BOOL contains = FALSE;
		hash_table_contains_type(drivers, driverId, DWORD, sizeof(DWORD), contains);
		if (!contains)
		{
			DWORD(*function)() = *(DWORD(**)())arguments;
			hash_table_emplace_type(drivers, driverId, DWORD, sizeof(DWORD), function, 4);
		}
		else
			ret = OBOS_DRIVER_ERROR_ALREADY_REGISTERED;
		break;
	case DRIVER_SYSCALL_LOG:
	{
		CSTRING message = *(CSTRING*)arguments;
		INT size = *(((PINT)arguments) + 1);
		printf("[Driver %d, LOG]: ", driverId);
		TerminalOutput(message, size);
		break;
	}
	default:
		break;
	}
	return ret;
}