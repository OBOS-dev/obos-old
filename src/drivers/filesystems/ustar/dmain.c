/*
	driver/filesystems/ustar/dmain.c

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>

#include <driver_api/enums.h>
#include <driver_api/syscall_interface.h>

#define TO_PBYTE(x) ((PBYTE)x)

UINT8_T* g_archivePosition = NULL;
SIZE_T g_archiveSize = 0;

static void ReadFile(CSTRING filename, STRING output, SIZE_T count);
static char FileExists(CSTRING filename, SIZE_T* size);

int _start()
{
	RegisterDriver(PASS_OBOS_API_PARS 1, SERVICE_TYPE_FILESYSTEM);
	
	if (GetMultibootModule(PASS_OBOS_API_PARS 3, (UINTPTR_T*)&g_archivePosition, &g_archiveSize))
		return 1; // Shouldn't ever happen.

	RegisterFileReadCallback(PASS_OBOS_API_PARS 1, ReadFile);
	RegisterFileExistsCallback(PASS_OBOS_API_PARS 1, FileExists);

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
SIZE_T strlen(CSTRING str)
{
	SIZE_T i = 0;
	for (; str[i]; i++);
	return i;
}
INT strcmp(CSTRING str1, CSTRING str2)
{
	if (str1 == str2)
		return 0;
	if (strlen(str1) != strlen(str2))
		return -2;
	return memcmp(str1, str2, strlen(str1));
}

static void ReadFile(CSTRING filename, STRING output, SIZE_T count)
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
char FileExists(CSTRING filename, SIZE_T* size)
{
	UINT8_T* iter = g_archivePosition;

	char ret = 0;

	while (!memcmp(iter + 257, "ustar", 6))
	{
		SIZE_T filesize = oct2bin(iter + 124, 11);
		if (!strcmp((CSTRING)iter, filename))
		{
			*size = filesize;
			BYTE type = *(iter + 156);
			ret = FILE_EXISTS_READ_ONLY;
			switch (type)
			{
			case '4':
			case '3':
				ret |= FILE_EXISTS_DEVICE;
				break;
			case '1':
				ret |= FILE_EXISTS_HARDLINK;
				break;
			case '2':
				ret |= FILE_EXISTS_SYMLINK;
				break;
			default:
				break;
			}
			break;
		}
		iter += (((filesize + 511) / 512) + 1) * 512;
	}
	return ret;

}
