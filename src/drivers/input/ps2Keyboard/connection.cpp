#include <types.h>

#include <driver_api/enums.h>
#include <driver_api/syscall_interface.h>

#include <syscalls/syscall_interface.h>

using namespace obos;

#pragma GCC push_options
#pragma GCC optimize("O3")
PVOID memcpy(PVOID dest, PCVOID src, SIZE_T nBytes)
{
	PBYTE _dest = (PBYTE)dest, _src = (PBYTE)src;
	for (SIZE_T i = 0; i < nBytes; i++)
		_dest[i] = _src[i];
	return dest;
}
SIZE_T strlen(CSTRING str)
{
	SIZE_T i = 0;
	for (; str[i]; i++);
	return i;
}
#pragma GCC pop_options

void HandleStatus(HANDLE handle , driverAPI::exitStatus status)
{
	if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
	{
		ConnectionClose(PASS_OBOS_API_PARS handle);
		ExitThread(PASS_OBOS_API_PARS GetLastError());
	}
}

extern driverAPI::driverHeader __attribute__((section(OBOS_DRIVER_HEADER_SECTION))) g_driverHeader;
extern BYTE g_keyBuffer[8192];
extern SIZE_T g_keyBufferPosition;

void ConnectionHandler(PVOID _handle)
{
	HANDLE handle = (HANDLE)_handle;

	driverAPI::exitStatus status = driverAPI::exitStatus::EXIT_STATUS_SUCCESS;

	while (1)
	{
		driverAPI::driver_commands command = driverAPI::driver_commands::OBOS_SERVICE_INVALID_SERVICE_COMMAND;
		status = ConnectionRecvData(PASS_OBOS_API_PARS handle, &command, sizeof(command));
		HandleStatus(handle, status);
		switch (command)
		{
		case obos::driverAPI::driver_commands::OBOS_SERVICE_GET_SERVICE_TYPE:
		{
			status = ConnectionSendData(PASS_OBOS_API_PARS handle, &g_driverHeader.service_type, sizeof(g_driverHeader.service_type));
			HandleStatus(handle, status);
			break;
		}
		case obos::driverAPI::driver_commands::OBOS_SERVICE_READ_CHARACTER:
		{
			status = ConnectionSendData(PASS_OBOS_API_PARS handle, &g_keyBuffer[0], sizeof(BYTE));
			HandleStatus(handle, status);
			if(g_keyBufferPosition)
			{
				memcpy(&g_keyBuffer[0], &g_keyBuffer[1], g_keyBufferPosition);
				g_keyBufferPosition--;
			}
			//g_keyBuffer[g_keyBufferPosition] = 0;
			break;
		}
		default:
			break;
		}
	}
}