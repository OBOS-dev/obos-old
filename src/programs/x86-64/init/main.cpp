/*
	programs/x86_64/init/main.cpp
	
	Copyright (c) 2023-2024 Omar Berrow
*/

#include <stdint.h>
#include <stddef.h>

#define OBOS_SYSCALL_IMPL
#include "syscall.h"
#include "liballoc.h"
#include "logger.h"

uintptr_t g_vAllocator = 0xffff'ffff'ffff'ffff;

void *memcpy(void* dest, const void* src, size_t size)
{
	uint8_t* _dest = (uint8_t*)dest;
	const uint8_t* _src = (uint8_t*)src;
	for (size_t i = 0; i < size; i++)
		_dest[i] = _src[i];
	return dest;
}
void* memzero(void* dest, size_t size)
{
	uint8_t* _dest = (uint8_t*)dest;
	for (size_t i = 0; i < size; i++)
		_dest[i] = 0;
	return dest;
}
size_t strlen(const char* str)
{
	if (!str)
		return 0;
	size_t ret = 0;
	for (; str[ret]; ret++);
	return ret;
}
size_t strCountToDelimiter(const char* str, char ch, bool stopAtZero = true)
{
	size_t ret = 0;
	for (; str[ret] != ch && (str[ret] && stopAtZero); ret++);
	return ret;
}
bool memcmp(const void* blk1, const void* blk2, size_t size)
{
	uint8_t* _blk1 = (uint8_t*)blk1;
	uint8_t* _blk2 = (uint8_t*)blk2;
	for (size_t i = 0; i < size; i++)
		if (_blk1[i] != _blk2[i])
			return false;
	return true;
}
bool strcmp(const char* str1, const char* str2)
{
	if (strlen(str1) != strlen(str2))
		return false;
	return memcmp(str1, str2, strlen(str1));
}
// Doesn't work with normal file handles, only with keyboard file handles.
char* getline(uintptr_t khandle)
{
	char ch[3] = {};
	size_t size = 0;
	char* res = (char*)malloc(++size);
	res[0] = 0;
	while (1)
	{
		while (Eof(khandle));
		ReadFile(khandle, ch, 2, false);
		if (!(ch[1] & (1 << 0)))
		{
			if (ch[0] == '\n')
				break;
			if (ch[0] == '\b')
			{
				if (size == 1)
					continue;
				ConsoleOutput(ch);
				res = (char*)realloc(res, size -= 1);
				res[size - 1] = 0;
				continue;
			}
			ConsoleOutput(ch);
			res = (char*)realloc(res, ++size);
			res[size - 2] = ch[0];
			res[size - 1] = 0;
		}
		ch[0] = ch[1] = ch[2] = 0;
	}
	printf("\n");
	return res;
}

void pageFaultHandler()
{
	uintptr_t cr2 = 0;
	asm volatile("mov %%cr2, %0;" : "=r"(cr2) : : );
	printf("\nPage fault! CR2: 0x%p\n", cr2);
	SwapBuffers();
	while (1);
}
static bool isNumber(char ch)
{
	char temp = ch - '0';
	return temp >= 0 && temp < 10;
}
static bool isNumber(const char* str, size_t size = 0)
{
	if (!size)
		size = strlen(str);
	for (size_t i = 0; i < size; i++)
		if (!isNumber(str[i]))
			return false;
	return true;
}
void dividePathToTokens(const char* filepath, const char**& tokens, size_t& nTokens, bool useOffset = true)
{
	for (; filepath[0] == '/'; filepath++);
	nTokens = 1;
	for (size_t i = 0; filepath[i]; i++)
		nTokens += filepath[i] == '/';
	for (int i = strlen(filepath) - 1; filepath[i] == '/'; i--)
		nTokens--;
	tokens = (const char**)(new char* [nTokens]);
	tokens[0] = filepath;
	for (size_t i = 0, nTokensCounted = 0; filepath[i] && nTokensCounted < nTokens; i++)
	{
		if (filepath[i] == '/' || i == 0)
		{
			if (!useOffset)
			{
				size_t szCurrentToken = strCountToDelimiter(filepath + i + (i != 0), '/');
				tokens[nTokensCounted++] = (const char*)memcpy(memzero(new char[szCurrentToken + 1], szCurrentToken + 1), filepath + i + (i != 0), szCurrentToken);
			}
			else
			{
				tokens[nTokensCounted++] = filepath + i + (i != 0);
			}
		}
	}
}
//static uint64_t dec2bin(const char* str, size_t size)
//{
//	uint64_t ret = 0;
//	for (size_t i = 0; i < size; i++)
//	{
//		if ((str[i] - '0') < 0 || (str[i] - '0') > 9)
//			continue;
//		ret *= 10;
//		ret += str[i] - '0';
//	}
//	return ret;
//}

void hexdump(void* _buff, size_t nBytes, const size_t width = 31);

char* relativePathToAbsolute(const char* filepath, const char* currentDirectory)
{
	size_t nTokens = 0;
	const char** tokens = nullptr;
	dividePathToTokens(filepath, tokens, nTokens, false);
	if (!nTokens)
		return nullptr;
	if (strCountToDelimiter(tokens[0], ':') != strlen(tokens[0]) && currentDirectory)
	{
		auto ret = relativePathToAbsolute(filepath, nullptr);
		for (size_t i = 0; i < nTokens; i++)
			delete[] tokens[i];
		delete[] tokens;
		return ret;
	}
	// Process the path.
	if (currentDirectory)
	{
		const char** newTokens = nullptr;
		size_t nNewTokens = 0;
		dividePathToTokens(currentDirectory, newTokens, nNewTokens, false);
		newTokens = (const char**)realloc(newTokens, (nNewTokens + nTokens) * sizeof(const char*));
		for (size_t i = nNewTokens; i < (nTokens + nNewTokens); i++)
			newTokens[i] = tokens[i - nNewTokens];
		delete[] tokens;
		nTokens += nNewTokens;
		tokens = newTokens;
	}
	for (size_t i = 0, nContinuousDoubleDots = 0; i < nTokens; i++)
	{
		if (strcmp(tokens[i], "."))
		{
			delete[] (tokens[i]);
			tokens[i] = nullptr;
		}
		if (strcmp(tokens[i], ".."))
		{
			nContinuousDoubleDots++;
			delete[] (tokens[i]);
			tokens[i] = nullptr;
			uint32_t tokenToDestroy = (i + 1 - nContinuousDoubleDots * 2);
			if (tokenToDestroy > 0 && tokenToDestroy < nTokens)
			{
				const char*& path = tokens[tokenToDestroy];
				delete[] path;
				path = nullptr;
			}
		}
		else
			nContinuousDoubleDots = 0;
	}
	char* path = nullptr;
	size_t szPath = 0;
	for (size_t i = 0; i < nTokens; i++)
	{
		if (!tokens[i])
			continue;
		size_t tokenLen = strlen(tokens[i]);
		path = (char*)realloc(path, szPath + tokenLen + 1);
		memcpy(path + szPath, tokens[i], tokenLen);
		szPath += strlen(tokens[i]);
		path[szPath++] = '/';
		delete[] tokens[i];
	}
	path = (char*)realloc(path, ++szPath);
	path[szPath - 1] = 0;
	delete[] tokens;
	return path;
}

int main(char* path)
{
	char* bootRootDirectory = (char*)memcpy(path + strlen(path) + 1, path, strCountToDelimiter(path, '/', true) + 1);
	char* currentDirectory = bootRootDirectory;
	ClearConsole(0);
	RegisterSignal(0, (uintptr_t)pageFaultHandler);
	uintptr_t keyboardHandle = MakeFileHandle();
	OpenFile(keyboardHandle, "K0", 0);
	while (1)
	{
		printf("%.*s>", strlen(currentDirectory) - 1, currentDirectory);
		SwapBuffers();
		char* currentLine = getline(keyboardHandle);
		size_t commandSz = strCountToDelimiter(currentLine, ' ', true);
		char* command = (char*)memcpy(new char[commandSz + 1], currentLine, commandSz);
		char* argPtr = currentLine + commandSz + 1;
		if (strcmp(command, "echo"))
		{
			printf("%s\n", argPtr);
		}
		else if (strcmp(command, "cat"))
		{
			char* filepath = relativePathToAbsolute(argPtr, currentDirectory);
			if (filepath[(strlen(filepath) - 1)] == '/')
				filepath[(strlen(filepath) - 1)] = 0;
			uintptr_t filehandle = MakeFileHandle();
			if (!OpenFile(filehandle, filepath, 1))
			{
				printf("Could not open %s. GetLastError: %d\n", filepath ? filepath : "(null)", GetLastError());
				if (filepath != nullptr)
					delete[] filepath;
				InvalidateHandle(filehandle);
				goto done;
			}
			size_t filesize = GetFilesize(filehandle);
			char* buff = new char[filesize + 1];
			buff[filesize] = 0;
			ReadFile(filehandle, buff, filesize, false);
			ConsoleOutput(buff);
			ConsoleOutput("\n");
			CloseFileHandle(filehandle);
			InvalidateHandle(filehandle);
			delete[] filepath;
			delete[] buff;
		}
		else if (strcmp(command, "shutdown"))
		{
			Shutdown();
		}
		else if (strcmp(command, "reboot"))
		{
			Reboot();
		}
		else if (strcmp(command, "broot"))
		{
			printf("%s\n", bootRootDirectory);
		}
		else if (strcmp(command, "cd"))
		{
			if (!strlen(argPtr))
			{
				printf("%s\n", currentDirectory);
				goto done;
			}
			char* newCurrentDirectory = relativePathToAbsolute(argPtr, currentDirectory);
			uintptr_t directoryIterator = MakeDirectoryIterator();
			if (!DirectoryIteratorOpen(directoryIterator, newCurrentDirectory))
			{
				delete[] newCurrentDirectory;
				InvalidateHandle(directoryIterator);
				goto done;
			}
			DirectoryIteratorClose(directoryIterator);
			InvalidateHandle(directoryIterator);
			if (bootRootDirectory != currentDirectory)
				delete[] currentDirectory;
			currentDirectory = newCurrentDirectory;
		}
		else if (strcmp(command, "ls"))
		{
			const char* dirToList = strlen(argPtr) ? relativePathToAbsolute(argPtr, currentDirectory) : currentDirectory;
			printf("Listing of directory %s\n", dirToList);
			uintptr_t directoryIterator = MakeDirectoryIterator();
			if (!DirectoryIteratorOpen(directoryIterator, dirToList))
			{
				printf("Could not open directory %s. GetLastError: %d\n", dirToList, GetLastError());
				if (currentDirectory != dirToList)
					delete[] dirToList;
				goto done;
			}
			for (; !DirectoryIteratorEnd(directoryIterator, nullptr); DirectoryIteratorNext(directoryIterator))
			{
				char* currentPath = nullptr;
				size_t szCurrentPath = 0;
				DirectoryIteratorGet(directoryIterator, currentPath, &szCurrentPath);
				DirectoryIteratorGet(directoryIterator, (char*)memzero(currentPath = new char[szCurrentPath + 1], szCurrentPath), &szCurrentPath);
				printf("%s\n", currentPath);
				delete[] currentPath;
			}
			if (currentDirectory != dirToList)
				delete[] dirToList;
			DirectoryIteratorClose(directoryIterator);
			InvalidateHandle(directoryIterator);
		}
		else if (strcmp(command, "hexdump"))
		{
			char* filepath = relativePathToAbsolute(argPtr, currentDirectory);
			if (filepath[(strlen(filepath) - 1)] == '/')
				filepath[(strlen(filepath) - 1)] = 0;
			uintptr_t filehandle = MakeFileHandle();
			if (!OpenFile(filehandle, filepath, 1))
			{
				printf("Could not open %s. GetLastError: %d\n", filepath ? filepath : "(null)", GetLastError());
				if (filepath != nullptr)
					delete[] filepath;
				InvalidateHandle(filehandle);
				goto done;
			}
			size_t filesize = GetFilesize(filehandle);
			char* buff = new char[filesize + 1];
			buff[filesize] = 0;
			ReadFile(filehandle, buff, filesize, false);
			hexdump(buff, filesize, 16);
			ConsoleOutput("\n");
			CloseFileHandle(filehandle);
			InvalidateHandle(filehandle);
			delete[] filepath;
			delete[] buff;
		}
		else
			printf("Invalid command %s.\n", command);
		done:
		free(currentLine);
		delete[] command;
	}
	return 0;
}

extern "C" void _start(char* path)
{
	g_vAllocator = CreateVirtualAllocator();
	InitializeConsole();
	struct
	{
		alignas(0x10) uint32_t exitCode;
	} par{};
	par.exitCode = main(path);
	if (par.exitCode != 0)
		printf("[Init] main failed with exit code %d.\n", par.exitCode);
	InvalidateHandle(g_vAllocator);
	syscall(12, &par); // ExitThread
}

typedef uint8_t byte;

void hexdump(void* _buff, size_t nBytes, const size_t width)
{
	bool printCh = false;
	byte* buff = (byte*)_buff;
	printf("         Address: ");
	for (byte i = 0; i < ((byte)width) + 1; i++)
		printf("%02x ", i);
	printf("\n%016x: ", buff);
	for (size_t i = 0, chI = 0; i < nBytes; i++, chI++)
	{
		if (printCh)
		{
			char ch = buff[i];
			switch (ch)
			{
			case '\n':
			case '\t':
			case '\r':
			case '\b':
			case '\a':
			case '\0':
			{
				ch = '.';
				break;
			}
			default:
				break;
			}
			printf("%c", ch);
		}
		else
			printf("%02x ", buff[i]);
		if (chI == static_cast<size_t>(width + (!(i < (width + 1)) || printCh)))
		{
			chI = 0;
			if (!printCh)
				i -= (width + 1);
			else
				printf(" |\n%016x: ", &buff[i + 1]);
			printCh = !printCh;
			if (printCh)
				printf("\t| ");
		}
	}
}