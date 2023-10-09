/*
	driver/filesystems/ustar/connection.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>

#include <driver_api/enums.h>
#include <driver_api/syscall_interface.h>

#include <syscalls/syscall_interface.h>

#include <error.h>

#include <stddef.h>

using namespace obos;
obos::driverAPI::fileExistsReturn FileExists(CSTRING filename, SIZE_T* size);
void ReadFile(CSTRING filename, STRING output, SIZE_T count);

[[nodiscard]] PVOID operator new(size_t count) noexcept
{
	PVOID ret = nullptr;
	HeapAllocate(PASS_OBOS_API_PARS & ret, count);
	return ret;
}
[[nodiscard]] PVOID operator new[](size_t count) noexcept
{
	return operator new(count);
}
VOID operator delete(PVOID block) noexcept
{
	HeapFree(PASS_OBOS_API_PARS block);
}
VOID operator delete[](PVOID block) noexcept
{
	HeapFree(PASS_OBOS_API_PARS block);
}
VOID operator delete(PVOID block, size_t)
{
	HeapFree(PASS_OBOS_API_PARS block);
}
VOID operator delete[](PVOID block, size_t)
{
	HeapFree(PASS_OBOS_API_PARS block);
}

void ConnectionHandler(PVOID _handle)
{
	HANDLE handle = (HANDLE)_handle;
	
	driverAPI::exitStatus status = driverAPI::exitStatus::EXIT_STATUS_SUCCESS;

	while (1)
	{
		SetLastError(OBOS_ERROR_NO_ERROR);
		driverAPI::driver_commands command = driverAPI::driver_commands::OBOS_SERVICE_INVALID_SERVICE_COMMAND;
		status = ConnectionRecvData(PASS_OBOS_API_PARS handle, &command, sizeof(command));
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
			status = ConnectionRecvData(PASS_OBOS_API_PARS handle, &filepathSize, sizeof(filepathSize));
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
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
				break;
			}
			PBYTE buffer = new BYTE[filepathSize];
			status = ConnectionRecvData(PASS_OBOS_API_PARS handle, buffer, filepathSize);
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				delete[] buffer;
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
			}
			status = ConnectionRecvData(PASS_OBOS_API_PARS handle, nullptr, sizeof(UINT64_T) + sizeof(BYTE));
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				delete[] buffer;
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
			}
			UINT64_T response[3] = { 0,0,0 };
			response[2] = (UINT64_T)FileExists((CSTRING)buffer, &response[0]);
			response[1] = 0;
			delete[] buffer;
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
		{
			SIZE_T filepathSize = 0;
			status = ConnectionRecvData(PASS_OBOS_API_PARS handle, &filepathSize, sizeof(filepathSize));
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
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
				break;
			}
			CSTRING filepath = new CHAR[filepathSize];
			status = ConnectionRecvData(PASS_OBOS_API_PARS handle, (PBYTE)filepath, filepathSize);

			SIZE_T filesize = 0;
			FileExists(filepath, &filesize);
			STRING response = new CHAR[filesize + sizeof(SIZE_T)];
			ReadFile(filepath, response + sizeof(SIZE_T), filesize);
			SIZE_T* firstRetValue = (SIZE_T*)response;
			*firstRetValue = filesize;
			status = ConnectionSendData(PASS_OBOS_API_PARS handle, response, filesize + sizeof(SIZE_T));
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				delete[] filepath;
				delete[] response;
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
			}

			delete[] response;
			delete[] filepath;
			break;
		}
		default:
			break;
		}
	}
}