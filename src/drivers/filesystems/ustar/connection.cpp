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
extern UINT8_T* g_archivePosition;
extern SIZE_T g_archiveSize;
obos::driverAPI::fileExistsReturn FileExists(CSTRING filename, SIZE_T* size);
INT memcmp(PCVOID block1, PCVOID block2, SIZE_T size);
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

struct fileHandleData
{
	PBYTE archivePosition = nullptr;
};
HANDLE g_nextFileIteratorHandle = 0;
fileHandleData* g_handles = nullptr;

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
		{
			status = ConnectionRecvData(PASS_OBOS_API_PARS handle, nullptr, sizeof(UINT64_T) + sizeof(BYTE));
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
			}
			HANDLE ret = 0;
			if (!g_handles)
			{
				g_handles = new fileHandleData[1];
				g_nextFileIteratorHandle++;
			}
			for (HANDLE i = 0; i < g_nextFileIteratorHandle; i++)
				ret = i * (g_handles[i].archivePosition == 0);
			if(g_handles[ret].archivePosition)
				ret = g_nextFileIteratorHandle++;
			if(ret != 0)
			{
				fileHandleData* newFileHandleData = new fileHandleData[g_nextFileIteratorHandle];
				for (SIZE_T i = 0; i < g_nextFileIteratorHandle; i++)
					newFileHandleData[i] = g_handles[i];
				delete[] g_handles;
				g_handles = newFileHandleData;
			}
			g_handles[ret].archivePosition = g_archivePosition;
			status = ConnectionSendData(PASS_OBOS_API_PARS handle, &ret, sizeof(ret));
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
			}
			break;
		}
		case obos::driverAPI::driver_commands::OBOS_SERVICE_NEXT_FILE:
		{
			HANDLE fileHandle = 0;
			status = ConnectionRecvData(PASS_OBOS_API_PARS handle, &fileHandle, sizeof(HANDLE));
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
			}
			if (fileHandle >= g_nextFileIteratorHandle || !g_handles[fileHandle].archivePosition)
			{
				UINT64_T response = 1;
				status = ConnectionSendData(PASS_OBOS_API_PARS handle, nullptr, sizeof(UINT64_T) * 3);
				status = ConnectionSendData(PASS_OBOS_API_PARS handle, &response, sizeof(response));
				status = ConnectionSendData(PASS_OBOS_API_PARS handle, nullptr, 1);
				if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
				{
					ConnectionClose(PASS_OBOS_API_PARS handle);
					ExitThread(PASS_OBOS_API_PARS GetLastError());
				}
				break;
			}
			fileHandleData* data = g_handles + fileHandle;
			if (memcmp(data->archivePosition + 257, "ustar", 6) != 0)
			{
				UINT64_T response = 1;
				status = ConnectionSendData(PASS_OBOS_API_PARS handle, nullptr, sizeof(UINT64_T) * 3);
				status = ConnectionSendData(PASS_OBOS_API_PARS handle, &response, sizeof(response));
				status = ConnectionSendData(PASS_OBOS_API_PARS handle, nullptr, 1);
				if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
				{
					ConnectionClose(PASS_OBOS_API_PARS handle);
					ExitThread(PASS_OBOS_API_PARS GetLastError());
				}
				break;
			}
			SIZE_T filepathLen = strlen((CSTRING)data->archivePosition);
			PBYTE filepath = new BYTE[filepathLen + 1];
			memcpy(filepath, data->archivePosition, filepathLen);
			UINT64_T fileExistsResponse[3] = {};
			fileExistsResponse[2] = (UINT64_T)FileExists((CSTRING)filepath, &fileExistsResponse[0]);
			fileExistsResponse[1] = 0;
			status = ConnectionSendData(PASS_OBOS_API_PARS handle, &fileExistsResponse, sizeof(fileExistsResponse));
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
			}
			status = ConnectionSendData(PASS_OBOS_API_PARS handle, &filepathLen, sizeof(filepathLen));
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
			}
			status = ConnectionSendData(PASS_OBOS_API_PARS handle, filepath, filepathLen);
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
			}
			data->archivePosition += (((fileExistsResponse[0] + 511) / 512) + 1) * 512;
			break;
		}
		case obos::driverAPI::driver_commands::OBOS_SERVICE_CLOSE_FILE_ITERATOR:
		{
			HANDLE fileHandle = 0;
			status = ConnectionRecvData(PASS_OBOS_API_PARS handle, &fileHandle, sizeof(HANDLE));
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
			}
			if (fileHandle >= g_nextFileIteratorHandle || !g_handles[fileHandle].archivePosition)
			{
				UINT8_T response = 1;
				status = ConnectionSendData(PASS_OBOS_API_PARS handle, &response, sizeof(response));
				if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
				{
					ConnectionClose(PASS_OBOS_API_PARS handle);
					ExitThread(PASS_OBOS_API_PARS GetLastError());
				}
				break;
			}
			g_handles[fileHandle].archivePosition = nullptr;
			status = ConnectionSendData(PASS_OBOS_API_PARS handle, nullptr, 1);
			if (status != driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			{
				ConnectionClose(PASS_OBOS_API_PARS handle);
				ExitThread(PASS_OBOS_API_PARS GetLastError());
			}
			break;
		}
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