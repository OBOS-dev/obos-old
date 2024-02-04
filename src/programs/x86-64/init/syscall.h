/*
	programs/x86_64/init/main.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <stdint.h>
#include <stddef.h>

uintptr_t MakeFileHandle();
bool OpenFile(uintptr_t hnd, const char* path, uint32_t options);
bool ReadFile(uintptr_t hnd, char* buff, size_t nToRead, bool peek = false);
size_t GetFilesize(uintptr_t hnd);
bool Eof(uintptr_t hnd);
bool CloseFileHandle(uintptr_t hnd);

uintptr_t CreateVirtualAllocator(void* unused = nullptr);
void* VirtualAlloc(uintptr_t hnd, void* base, size_t size, uintptr_t flags);
void* VirtualFree(uintptr_t hnd, void* base, size_t size);

bool InitializeConsole();
void ConsoleOutput(const char* str);
void SetColour(uint32_t foregroundColour, uint32_t backgroundColour, bool clearConsole = false);
void GetColour(uint32_t* foregroundColour, uint32_t* backgroundColour);
void ClearConsole(uint32_t background);
void SwapBuffers();

uintptr_t MakeThreadHandle();
bool CreateThread(uintptr_t hnd, uint32_t priority, size_t stackSize, void(*entry)(uintptr_t), uintptr_t userdata, __uint128_t affinity, void* process, bool startPaused);
uint32_t GetThreadStatus(uintptr_t hnd);
uint32_t GetThreadExitCode(uintptr_t hnd);
bool CloseThreadHandle(uintptr_t hnd);

bool InvalidateHandle(uintptr_t hnd);

bool LoadModule(const uint8_t* data, size_t size);
uint32_t GetLastError();

bool RegisterSignal(uint32_t signal, uintptr_t handler);

uintptr_t MakeDirectoryIterator();
bool DirectoryIteratorOpen(uintptr_t hnd, const char* path);
bool DirectoryIteratorNext(uintptr_t hnd);
bool DirectoryIteratorPrevious(uintptr_t hnd);
bool DirectoryIteratorGet(uintptr_t hnd, char* oPath, size_t* oPathSize);
bool DirectoryIteratorEnd(uintptr_t hnd, bool* status);
bool DirectoryIteratorClose(uintptr_t hnd);
bool DirectoryIteratorGetParent(uintptr_t hnd, char* path, size_t* size);

[[noreturn]] void Shutdown();
[[noreturn]] void Reboot();

#ifdef OBOS_SYSCALL_IMPL
extern "C" uintptr_t syscall(uint64_t syscallNo, void* args);
uintptr_t MakeFileHandle()
{
	return syscall(13, nullptr);
}
bool OpenFile(uintptr_t hnd, const char* path, uint32_t options)
{
	struct _par
	{
		alignas(0x10) uintptr_t hnd;
		alignas(0x10) const char* path;
		alignas(0x10) uint32_t options;
	} pars{ hnd, path, options };
	return syscall(14, &pars);
}
bool ReadFile(uintptr_t hnd, char* buff, size_t nToRead, bool peek)
{
	struct _par
	{
		alignas(0x10) uintptr_t hnd;
		alignas(0x10) char* data;
		alignas(0x10) size_t nToRead;
		alignas(0x10) bool peek;
	} pars{ hnd, buff, nToRead, peek };
	return syscall(15, &pars);
}
size_t GetFilesize(uintptr_t hnd)
{
	uintptr_t pars[2] = { hnd, 0 };
	return syscall(19, &pars);
}
bool Eof(uintptr_t hnd)
{
	uintptr_t pars[2] = { hnd, 0 };
	return (bool)(uint8_t)syscall(16, &pars);
}
bool CloseFileHandle(uintptr_t hnd)
{
	uintptr_t pars[2] = { hnd, 0 };
	return syscall(22, &pars);
}

uintptr_t CreateVirtualAllocator(void* unused)
{
	unused = nullptr;
	struct
	{
		alignas(0x10) void* unused;
	} par{ unused };
	return syscall(39, &par);
}
void* VirtualAlloc(uintptr_t hnd, void* base, size_t size, uintptr_t flags)
{
	struct
	{
		alignas(0x10) uintptr_t hnd;
		alignas(0x10) void* base;
		alignas(0x10) size_t size;
		alignas(0x10) uintptr_t flags;
	} par{};
	par.hnd = hnd;
	par.base = base;
	par.size = size;
	par.flags = flags;
	return (void*)syscall(40, &par);
}
void* VirtualFree(uintptr_t hnd, void* base, size_t size)
{
	struct
	{
		alignas(0x10) uintptr_t hnd;
		alignas(0x10) void* base;
		alignas(0x10) size_t size;
	} par{};
	par.hnd = hnd;
	par.base = base;
	par.size = size;
	return (void*)syscall(41, &par);
}

bool InitializeConsole()
{
	return syscall(25, nullptr);
}
void ConsoleOutput(const char* str)
{
	size_t sz = 0;
	for (; str[sz]; sz++);
	struct
	{
		alignas(0x10) const char* str;
		alignas(0x10) size_t sz;
	} par{};
	par.str = str;
	par.sz = sz;
	syscall(26, &par);
}
void SetColour(uint32_t foregroundColour, uint32_t backgroundColour, bool clearConsole)
{
	struct _par
	{
		alignas(0x10) uint32_t par1;
		alignas(0x10) uint32_t par2;
		alignas(0x10) bool clearConsole;
	} pars{ foregroundColour, backgroundColour, clearConsole };
	syscall(32, &pars);
}
void GetColour(uint32_t* foregroundColour, uint32_t* backgroundColour)
{
	struct _par
	{
		alignas(0x10) uint32_t* par1;
		alignas(0x10) uint32_t* par2;
	} pars{ foregroundColour, backgroundColour };
	syscall(33, &pars);
}
void ClearConsole(uint32_t background)
{
	uint32_t foreground = 0;
	GetColour(&foreground, nullptr);
	SetColour(foreground, background, true);
}
void SwapBuffers()
{
	syscall(57, nullptr);
}

uintptr_t MakeThreadHandle()
{
	return syscall(0, nullptr);
}
bool CreateThread(uintptr_t hnd, uint32_t priority, size_t stackSize, void(*entry)(uintptr_t), uintptr_t userdata, __uint128_t affinity, void* process, bool startPaused)
{
	struct _pars
	{
		alignas(0x10) uintptr_t hnd;
		alignas(0x10) uint32_t priority;
		alignas(0x10) size_t stackSize;
		alignas(0x10) void(*entry)(uintptr_t);
		alignas(0x10) uintptr_t userdata;
		alignas(0x10) __uint128_t affinity;
		alignas(0x10) void* process;
		alignas(0x10) bool startPaused;
	} pars{
		hnd, priority, stackSize, entry, userdata, affinity, process, startPaused
	};
	return syscall(2, &pars);
}
uint32_t GetThreadStatus(uintptr_t hnd)
{
	uintptr_t pars[2] = { hnd, 0 };
	return syscall(7, &pars);
}
uint32_t GetThreadExitCode(uintptr_t hnd)
{
	uintptr_t pars[2] = { hnd, 0 };
	return syscall(8, &pars);
}
bool CloseThreadHandle(uintptr_t hnd)
{
	uintptr_t pars[2] = { hnd, 0 };
	return syscall(23, &pars);
}

bool InvalidateHandle(uintptr_t hnd)
{
	struct
	{
		alignas(0x10) uintptr_t hnd;
	} par{ hnd };
	return syscall(24, &par);
}

bool LoadModule(const uint8_t* data, size_t size)
{
	struct _pars
	{
		alignas(0x10) const uint8_t* data;
		alignas(0x10) size_t size;
	} pars = { data,size };
	return syscall(57, &pars);
}
uint32_t GetLastError()
{
	return syscall(55, nullptr);
}
bool RegisterSignal(uint32_t signal, uintptr_t handler)
{
	struct _par
	{
		alignas(0x10) uint32_t signal;
		alignas(0x10) uintptr_t uhandler;
	} par{ signal, handler };
	return syscall(57, &par);
}
[[noreturn]] void Shutdown()
{
	syscall(59, nullptr);
	while (1);
}
[[noreturn]] void Reboot()
{
	syscall(60, nullptr);
	while (1);
}

uintptr_t MakeDirectoryIterator()
{
	return syscall(61, nullptr);
}
bool DirectoryIteratorOpen(uintptr_t hnd, const char* path)
{
	struct _par
	{
		alignas(0x10) uintptr_t hnd;
		alignas(0x10) const char* path;
	} par{ hnd, path };
	return syscall(66, &par);
}
bool DirectoryIteratorNext(uintptr_t hnd)
{
	uintptr_t pars[2] = { hnd, 0 };
	return syscall(62, &pars);
}
bool DirectoryIteratorPrevious(uintptr_t hnd)
{
	uintptr_t pars[2] = { hnd, 0 };
	return syscall(63, &pars);
}
bool DirectoryIteratorGet(uintptr_t hnd, char* oPath, size_t* oPathSize)
{
	struct _par
	{
		alignas(0x10) uintptr_t hnd;
		alignas(0x10) char* oPath;
		alignas(0x10) size_t* oPathSize;
	} par{ hnd, oPath, oPathSize };
	return syscall(64, &par);
}
bool DirectoryIteratorEnd(uintptr_t hnd, bool* status)
{
	struct _par
	{
		alignas(0x10) uintptr_t hnd;
		alignas(0x10) bool* status;
	} par{ hnd, status };
	return syscall(65, &par);
}
bool DirectoryIteratorClose(uintptr_t hnd)
{
	uintptr_t pars[2] = { hnd, 0 };
	return syscall(67, &pars);
}
bool DirectoryIteratorGetParent(uintptr_t hnd, char* path, size_t* size)
{
	struct _par
	{
		alignas(0x10) uintptr_t hnd;
		alignas(0x10) char* oPath;
		alignas(0x10) size_t* oPathSize;
	} par{ hnd, path, size };
	return syscall(68, &par);
}
#endif