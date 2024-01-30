/*
	programs/x86_64/init/main.cpp
	
	Copyright (c) 2023-2024 Omar Berrow
*/

#include <stdint.h>
#include <stddef.h>

extern "C" uintptr_t syscall(uint64_t syscallNo, void* args);

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

void *memcpy(void* dest, const void* src, size_t size)
{
	uint8_t* _dest = (uint8_t*)dest;
	const uint8_t* _src = (uint8_t*)src;
	for (size_t i = 0; i < size; i++)
		_dest[i] = _src[i];
	return dest;
}
size_t strlen(const char* str)
{
	size_t ret = 0;
	for (; str[ret]; ret++);
	return ret;
}
size_t strCountToDelimiter(const char* str, char ch, bool stopAtZero)
{
	size_t ret = 0;
	for (; str[ret] != ch && (str[ret] && stopAtZero); ret++);
	return ret;
}
static char* itoa(intptr_t value, char* result, int base);
static char* itoa_unsigned(uintptr_t value, char* result, int base);

int main(char* path)
{
	char* bootRootDirectory = (char*)memcpy(path + strlen(path) + 1, path, strCountToDelimiter(path, '/', true) + 1);
	/*char* driverPath = (char*)memcpy(bootRootDirectory + strlen(bootRootDirectory), bootRootDirectory, strlen(bootRootDirectory));
	memcpy(driverPath + strlen(driverPath), "obos/acpiDriver", 16);*/
	ClearConsole(0);
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
	{
		ConsoleOutput("[Init] main failed with exit code: ");
		char str[12] = {};
		itoa(par.exitCode, str, 10);
		ConsoleOutput(str);
		ConsoleOutput("\n");
	}
	InvalidateHandle(g_vAllocator);
	syscall(12, &par); // ExitThread
}
static void strreverse(char* begin, int size)
{
	int i = 0;
	char tmp = 0;

	for (i = 0; i < size / 2; i++)
	{
		tmp = begin[i];
		begin[i] = begin[size - i - 1];
		begin[size - i - 1] = tmp;
	}
}
// Credit: http://www.strudel.org.uk/itoa/
static char* itoa_unsigned(uintptr_t value, char* result, int base)
{
	// check that the base if valid

	if (base < 2 || base > 16)
	{
		*result = 0;
		return result;
	}

	char* out = result;

	uintptr_t quotient = value;

	do
	{

		uintptr_t abs = /*(quotient % base) < 0 ? (-(quotient % base)) : */(quotient % base);

		*out = "0123456789abcdef"[abs];

		++out;

		quotient /= base;

	} while (quotient);

	strreverse(result, out - result);

	return result;

}
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