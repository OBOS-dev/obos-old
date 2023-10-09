/*
	driver/filesystems/ustar/dmain.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>

#include <driver_api/enums.h>
#include <driver_api/syscall_interface.h>

#include <syscalls/syscall_interface.h>

#define TO_PBYTE(x) ((PBYTE)x)

UINT8_T* g_archivePosition = NULL;
SIZE_T g_archiveSize = 0;

void IterateFiles(BOOL(*appendCallback)(CSTRING filename, SIZE_T bufSize, BYTE attrib));

#define DRIVER_ID 1

obos::driverAPI::driverHeader __attribute__((section(OBOS_DRIVER_HEADER_SECTION))) g_driverHeader = {
	.magicNumber = OBOS_DRIVER_HEADER_MAGIC,
	.driverId = DRIVER_ID,
	.service_type = obos::driverAPI::serviceType::OBOS_SERVICE_TYPE_FILESYSTEM,
	.infoFields = 0,
	.pciInfo = {}
};

void ConnectionHandler(PVOID _handle);

extern "C" int _start()
{
	if (GetMultibootModule(PASS_OBOS_API_PARS 2, (UINTPTR_T*)&g_archivePosition, &g_archiveSize) != obos::driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
		return 1; // Shouldn't ever happen.
	
	HANDLE connection = 0;
	HANDLE thread = 0;

	while (1)
	{
		if (ListenForConnections(PASS_OBOS_API_PARS &connection) != obos::driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			return GetLastError();

		CreateThread(PASS_OBOS_API_PARS &thread, 4, ConnectionHandler, (PVOID)connection, 0,0);
		CloseHandle(PASS_OBOS_API_PARS thread);
	}

	return 0;
}

int oct2bin(unsigned char* str, int size) {
	int n = 0;
	unsigned char* c = str;
	while (size-- > 0) {
		n *= 8;
		n += *c - '0';
		c++;
	}
	return n;
}

INT memcmp(PCVOID block1, PCVOID block2, SIZE_T size)
{
	if (block1 == block2)
		return 0;
	for (SIZE_T i = 0; i < size; i++)
		if (TO_PBYTE(block1)[i] < TO_PBYTE(block2)[i])
			return -1;
		else if (TO_PBYTE(block1)[i] > TO_PBYTE(block2)[i])
			return -1;
	return 0;
}
extern SIZE_T strlen(CSTRING str);
INT strcmp(CSTRING str1, CSTRING str2)
{
	if (str1 == str2)
		return 0;
	if (strlen(str1) != strlen(str2))
		return -2;
	return memcmp(str1, str2, strlen(str1));
}

void ReadFile(CSTRING filename, STRING output, SIZE_T count)
{
	UINT8_T* iter = g_archivePosition;

	while(!memcmp(iter + 257, "ustar", 6))
	{
		SIZE_T filesize = oct2bin(iter + 124, 11);
		if (!strcmp((CSTRING)iter, filename))
		{
			UINT8_T* filedata = iter + 512;
			for (SIZE_T i = 0; i < count; i++)
			{
				if (i >= filesize)
					return;
				output[i] = filedata[i];
			}
		}
		iter += (((filesize + 511) / 512) + 1) * 512;
	}
}

obos::driverAPI::fileExistsReturn MakeFileAttribFromType(char type)
{
	obos::driverAPI::fileExistsReturn ret = obos::driverAPI::fileExistsReturn::FILE_DOESNT_EXIST;
	switch (type)
	{
	case '1':
		ret = obos::driverAPI::fileExistsReturn::FILE_EXISTS_HARDLINK;
		break;
	case '2':
		ret = obos::driverAPI::fileExistsReturn::FILE_EXISTS_SYMLINK;
		break;
	case '5':
		ret = obos::driverAPI::fileExistsReturn::FILE_EXISTS_DIRECTORY;
		break;
	case 0:
	case '0':
		ret = obos::driverAPI::fileExistsReturn::FILE_EXISTS_FILE;
		break;
	default:
		break;
	}
	return ret;
}

obos::driverAPI::fileExistsReturn FileExists(CSTRING filename, SIZE_T* size)
{
	UINT8_T* iter = g_archivePosition;

	obos::driverAPI::fileExistsReturn ret;

	while (!memcmp(iter + 257, "ustar", 6))
	{
		SIZE_T filesize = oct2bin(iter + 124, 11);
		if (!strcmp((CSTRING)iter, filename))
		{
			*size = filesize;
			BYTE type = *(iter + 156);
			ret = MakeFileAttribFromType(type);
			break;
		}
		iter += (((filesize + 511) / 512) + 1) * 512;
	}
	return ret;
}
void IterateFiles(BOOL(*)(CSTRING filename, SIZE_T bufSize, BYTE attrib))
{
	UINT8_T* iter = g_archivePosition;

	while (!memcmp(iter + 257, "ustar", 6))
	{
		SIZE_T filesize = oct2bin(iter + 124, 11);
		/*if (!appendCallback((CSTRING)iter, strlen((CSTRING)iter), MakeFileAttribFromType(*(iter + 156))))
			return;*/
		iter += (((filesize + 511) / 512) + 1) * 512;
	}
}
