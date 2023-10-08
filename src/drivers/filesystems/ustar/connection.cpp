/*
	driver/filesystems/ustar/connection.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>

#include <driver_api/enums.h>
#include <driver_api/syscall_interface.h>

#include <syscalls/syscall_interface.h>

#include <error.h>

using namespace obos;
extern obos::driverAPI::fileExistsReturn FileExists(CSTRING filename, SIZE_T* size);

void ConnectionHandler(PVOID _handle)
{
	HANDLE handle = (HANDLE)_handle;
	
	driverAPI::exitStatus status = driverAPI::exitStatus::EXIT_STATUS_SUCCESS;

	while (1)
	{
		SetLastError(OBOS_ERROR_NO_ERROR);
		driverAPI::driver_commands command = driverAPI::driver_commands::OBOS_SERVICE_INVALID_SERVICE_COMMAND;
		status = ConnectionRecvData(PASS_OBOS_API_PARS handle, &command, sizeof(command));
		if (GetLastError() == OBOS_ERROR_BUFFER_TOO_SMALL)
			continue;
		if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
		{
			ConnectionClose(PASS_OBOS_API_PARS handle);
			ExitThread(PASS_OBOS_API_PARS GetLastError());
		}
		switch (command)
		{
		case obos::driverAPI::driver_commands::OBOS_SERVICE_GET_SERVICE_TYPE:
		{
			driverAPI::serviceType response = driverAPI::serviceType::OBOS_SERVICE_TYPE_INITRD_FILESYSTEM;
			status = ConnectionSendData(PASS_OBOS_API_PARS handle, &response, sizeof(response));
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
			}
			break;
		}
		case obos::driverAPI::driver_commands::OBOS_SERVICE_QUERY_FILE_DATA:
		{
			// Read the filepath size.
			SIZE_T filepathSize = 0;
			while (1)
			{
				status = ConnectionRecvData(PASS_OBOS_API_PARS handle, &filepathSize, sizeof(filepathSize));
				if (GetLastError() == OBOS_ERROR_BUFFER_TOO_SMALL)
				{
					continue;
				}
				if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
				{
					ConnectionClose(PASS_OBOS_API_PARS handle);
					ExitThread(PASS_OBOS_API_PARS GetLastError());
				}
				else
					break;
			}
			if (!filepathSize || filepathSize > 100)
			{
				UINT64_T response[3] = { 0,0, (UINT32_T)driverAPI::fileExistsReturn::FILE_DOESNT_EXIST };
				status = ConnectionSendData(PASS_OBOS_API_PARS handle, &response, sizeof(response));
				if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
				{
					ConnectionClose(PASS_OBOS_API_PARS handle);
					ExitThread(PASS_OBOS_API_PARS GetLastError());
				}
				continue;
			}
			const SIZE_T filepathSizePages = ((filepathSize + 0xfff) & ~0xfff) / 4096;
			PBYTE buffer = (PBYTE)VirtualAlloc(PASS_OBOS_API_PARS nullptr, filepathSizePages, 2);
			while (1)
			{
				status = ConnectionRecvData(PASS_OBOS_API_PARS handle, buffer, filepathSize);
				if (GetLastError() == OBOS_ERROR_BUFFER_TOO_SMALL)
				{
					continue;
				}
				if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
				{
					VirtualFree(PASS_OBOS_API_PARS buffer, filepathSizePages);
					ConnectionClose(PASS_OBOS_API_PARS handle);
					ExitThread(PASS_OBOS_API_PARS GetLastError());
				}
				else
					break;
			}
			while (1)
			{
				status = ConnectionRecvData(PASS_OBOS_API_PARS handle, nullptr, sizeof(UINT64_T) + sizeof(BYTE));
				if (GetLastError() == OBOS_ERROR_BUFFER_TOO_SMALL)
				{
					continue;
				}
				if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
				{
					VirtualFree(PASS_OBOS_API_PARS buffer, filepathSizePages);
					ConnectionClose(PASS_OBOS_API_PARS handle);
					ExitThread(PASS_OBOS_API_PARS GetLastError());
				}
				else
					break;
			}
			SIZE_T filesize = 0;
			UINT64_T response[3] = { 0,0,0 };
			response[2] = (UINT64_T)FileExists((CSTRING)buffer, &response[0]);
			response[1] = 0;
			VirtualFree(PASS_OBOS_API_PARS buffer, filepathSizePages);
			status = ConnectionSendData(PASS_OBOS_API_PARS handle, &response, sizeof(response));
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
			}
			break;
		}
		case obos::driverAPI::driver_commands::OBOS_SERVICE_MAKE_FILE_ITERATOR:
			break;
		case obos::driverAPI::driver_commands::OBOS_SERVICE_NEXT_FILE:
			break;
		case obos::driverAPI::driver_commands::OBOS_SERVICE_READ_FILE:
			break;
		default:
			break;
		}
	}
}