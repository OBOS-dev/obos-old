// This file is released to the public domain.
// Example syscall code for oboskrnl on x86-64
// Compile with x86_64-elf-g++.

#include <stdint.h>
#include <stddef.h>

extern "C" uintptr_t syscall(uint64_t syscallNo, void* args);

static char* itoa(intptr_t value, char* result, int base);

uintptr_t g_vAllocator = 0xffff'ffff'ffff'ffff;

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
bool ReadFile(uintptr_t hnd, char* buff, size_t nToRead, bool peek = false)
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
bool CloseFileHandle(uintptr_t hnd)
{
	uintptr_t pars[2] = { hnd, 0 };
	return syscall(22, &pars);
}

uintptr_t CreateVirtualAllocator(void* unused = nullptr)
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
void SetColour(uint32_t foregroundColour, uint32_t backgroundColour, bool clearConsole = false)
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

void *memcpy(void* dest, const void* src, size_t size)
{
	uint8_t* _dest = (uint8_t*)dest;
	const uint8_t* _src = (uint8_t*)src;
	for (size_t i = 0; i < size; i++)
		_dest[i] = _src[i];
	return dest;
}

static uint32_t test()
{
	// Make a file handle
	uintptr_t fileHandle = MakeFileHandle();
	// Open a file on that handle.
	if (!OpenFile(fileHandle, "1:/splash.txt", 1))
		return 1;
	// Get the file size.
	size_t fileSize = GetFilesize(fileHandle);
	// Allocate fileSize + 1, rounded up to the page size pages.
	char* data = (char*)VirtualAlloc(g_vAllocator, nullptr, fileSize + 1, 0);
	// Read the entirety of the file.
	ReadFile(fileHandle, data, fileSize);
	// Set the console foreground text colour to blue and output the file contents.
	SetColour(0x003399FF, 0, false);
	ConsoleOutput(data);
	SetColour(0x00CCCCCC, 0, false);
	// Free the buffer.
	VirtualFree(g_vAllocator, data, fileSize + 1);
	// Close the file
	CloseFileHandle(g_vAllocator);
	// Invalidate the handle
	InvalidateHandle(g_vAllocator);
	return 0;
}
void thrStart(uintptr_t)
{
	uint32_t exitCode = test();
exit:
	struct
	{
		alignas(0x10) uint32_t exitCode;
	} par{};
	par.exitCode = exitCode;
	syscall(12, &par); // ExitThread
}
int main()
{
	ClearConsole(0);
	ConsoleOutput("Starting the test thread.\n");
	uintptr_t thrHandle = MakeThreadHandle();
	CreateThread(thrHandle, 4, 0, thrStart, 0, 0, nullptr, false);
	ConsoleOutput("Waiting for the test thread...\n");
	while (!(GetThreadStatus(thrHandle) & 1))
		__asm("pause");
	ConsoleOutput("The test thread exited with exit code 0x");
	char res[9] = {};
	itoa(GetThreadExitCode(thrHandle), res, 16);
	ConsoleOutput(res);
	ConsoleOutput("\n");
	CloseThreadHandle(thrHandle);
	InvalidateHandle(thrHandle);
	return 0;
}

extern "C" void _start()
{
	g_vAllocator = CreateVirtualAllocator();
	InitializeConsole();
	struct
	{
		alignas(0x10) uint32_t exitCode;
	} par{};
	par.exitCode = main();
	InvalidateHandle(g_vAllocator);
	syscall(12, &par); // ExitThread
}

// Credit: http://www.strudel.org.uk/itoa/
static char* itoa(intptr_t value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, * ptr1 = result, tmp_char;
	intptr_t tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}