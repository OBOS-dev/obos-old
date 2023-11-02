#include <int.h>

#include <arch/x86_64/syscall/syscall_interface.h>

#include <stdbool.h>

extern uint64_t Syscall(uint16_t call, void* pars);
__asm__("Syscall: mov %rdi, %rax; mov %rsi, %rdi; int $0x32; ret");

// Virtual memory manager syscalls.
void* VirtualAlloc(void* base, size_t size, uintptr_t flags)
{
	struct
	{
		__attribute__((aligned(8))) void* base;
		__attribute__((aligned(8))) size_t size;
		__attribute__((aligned(8))) uintptr_t flags;
	} par = { base,size,flags };
	return (void*)Syscall(0, (void*)&par);
}
bool VirtualFree(void* base, size_t size)
{
	struct
	{
		__attribute__((aligned(8))) void* base;
		__attribute__((aligned(8))) size_t size;
	} par = { base,size };
	return Syscall(1, (void*)&par);
}
bool VirtualProtect(void* base, size_t size, uintptr_t flags)
{
	struct
	{
		__attribute__((aligned(8))) void* base;
		__attribute__((aligned(8))) size_t size;
		__attribute__((aligned(8))) uintptr_t flags;
	} par = { base,size,flags };
	return Syscall(2, (void*)&par);
}
bool VirtualGetProtection(void* base, size_t size, uintptr_t* oFlags)
{
	struct
	{
		__attribute__((aligned(8))) void* base;
		__attribute__((aligned(8))) size_t size;
		__attribute__((aligned(8))) uintptr_t *oFlags;
	} par = { base,size,oFlags };
	Syscall(3, (void*)&par);
}

// Console manipulation syscalls.
void AllocConsole()
{
	Syscall(4, NULL);
}
void ConsoleOutputCharacter(char ch)
{
	Syscall(5, &ch);
}
void ConsoleOutputCharacterAt(char ch, uint32_t x, uint32_t y)
{
	struct
	{
		__attribute__((aligned(8))) char ch;
		__attribute__((aligned(8))) uint32_t x;
		__attribute__((aligned(8))) uint32_t y;
	} par = { ch,x,y };
	Syscall(6, &par);
}
void ConsoleOutputCharacterAtWithColour(char ch, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t x, uint32_t y)
{
	struct
	{
		__attribute__((aligned(8))) char ch;
		__attribute__((aligned(8))) uint32_t foregroundColour;
		__attribute__((aligned(8))) uint32_t backgroundColour;
		__attribute__((aligned(8))) uint32_t x;
		__attribute__((aligned(8))) uint32_t y;
	} par = { ch, foregroundColour,backgroundColour, x,y };
	Syscall(7, &par);
}
void ConsoleOutputString(const char* string)
{
	Syscall(8, &string);
}
void ConsoleOutputStringSz(const char* string, size_t size)
{
	struct
	{
		__attribute__((aligned(8))) const char* string;
		__attribute__((aligned(8))) size_t size;
	} par = { string,size };
	Syscall(9, &par);
}
void ConsoleSetPosition(uint32_t x, uint32_t y)
{
	struct
	{
		__attribute__((aligned(8))) uint32_t x;
		__attribute__((aligned(8))) uint32_t y;
	} par = { x,y };
	Syscall(10, &x);
}
void ConsoleGetPosition(uint32_t* x, uint32_t* y)
{
	struct
	{
		__attribute__((aligned(8))) uint32_t* x;
		__attribute__((aligned(8))) uint32_t* y;
	} par = { x,y };
	Syscall(11, &par);
}
void ConsoleSetColour(uint32_t foregroundColour, uint32_t backgroundColour, bool clearConsole)
{
	struct
	{
		__attribute__((aligned(8))) uint32_t foregroundColour;
		__attribute__((aligned(8))) uint32_t backgroundColour;
		__attribute__((aligned(8))) bool clearConsole;
	} par = { foregroundColour, backgroundColour, clearConsole };
	Syscall(12, &par);
}
void ConsoleGetColour(uint32_t* foregroundColour, uint32_t* backgroundColour)
{
	struct
	{
		__attribute__((aligned(8))) uint32_t *foregroundColour;
		__attribute__((aligned(8))) uint32_t *backgroundColour;
	} par = { foregroundColour, backgroundColour };
	Syscall(13, &par);
}
void ConsoleSetFont(uint8_t* font)
{
	Syscall(14, &font);
}
void ConsoleGetFont(uint8_t** oFont)
{
	Syscall(15, &oFont);
}
void ConsoleSetFramebuffer(con_framebuffer framebuffer)
{
	Syscall(16, &framebuffer);
}
void ConsoleGetFramebuffer(con_framebuffer* framebuffer)
{
	Syscall(17, &framebuffer);
}
void ConsoleGetConsoleBounds(uint32_t* horizontal, uint32_t* vertical)
{
	struct
	{
		__attribute__((aligned(8))) uint32_t* horizontal;
		__attribute__((aligned(8))) uint32_t* vertical;
	} par = { horizontal, vertical };
	Syscall(18, &par);
}
void FreeConsole()
{
	Syscall(19, NULL);
}

// Thread manipulation syscalls.

uintptr_t MakeThreadObject()
{
	return Syscall(20, NULL);
}
bool CreateThread(uintptr_t _this, uint32_t priority, size_t stackSize, void(*entry)(uintptr_t), uintptr_t userdata, bool startPaused)
{
	struct
	{
		__attribute__((aligned(8))) uintptr_t _this; 
		__attribute__((aligned(8))) uint32_t priority; 
		__attribute__((aligned(8))) size_t stackSize;
		__attribute__((aligned(8))) void(*entry)(uintptr_t);
		__attribute__((aligned(8))) uintptr_t userdata;
		__attribute__((aligned(8))) bool startPaused;
	} par = { _this,priority,stackSize,entry,userdata,startPaused };
	return Syscall(21, &par);
}
bool OpenThread(uintptr_t _this, uint32_t tid)
{
	struct
	{
		__attribute__((aligned(8))) uintptr_t _this;
		__attribute__((aligned(8))) uint32_t tid;
	} par = { _this,tid };
	return Syscall(22, &par);
}
bool PauseThread(uintptr_t _this)
{
	struct
	{
		__attribute__((aligned(8))) uintptr_t _this;
	} par = { _this };
	return Syscall(23, &par);
}
bool ResumeThread(uintptr_t _this)
{
	struct
	{
		__attribute__((aligned(8))) uintptr_t _this;
	} par = { _this };
	return Syscall(24, &par);
}
bool SetThreadPriority(uintptr_t _this, uint32_t priority)
{
	struct
	{
		__attribute__((aligned(8))) uintptr_t _this;
		__attribute__((aligned(8))) uint32_t priority;
	} par = { _this,priority };
	return Syscall(25, &par);
}
bool TerminateThread(uintptr_t _this)
{
	struct
	{
		__attribute__((aligned(8))) uintptr_t _this;
	} par = { _this };
	return Syscall(26, &par);
}
uint32_t GetThreadStatus(uintptr_t _this)
{
	struct
	{
		__attribute__((aligned(8))) uintptr_t _this;
	} par = { _this };
	return Syscall(27, &par);
}
uint32_t GetThreadExitCode(uintptr_t _this)
{
	struct
	{
		__attribute__((aligned(8))) uintptr_t _this;
	} par = { _this };
	return Syscall(28, &par);
}
uint32_t GetThreadLastError(uintptr_t _this)
{
	struct
	{
		__attribute__((aligned(8))) uintptr_t _this;
	} par = { _this };
	return Syscall(29, &par);
}
bool CloseThread(uintptr_t _this)
{
	struct
	{
		__attribute__((aligned(8))) uintptr_t _this;
	} par = { _this };
	return Syscall(30, &par);
}
bool FreeThread(uintptr_t _this)
{
	struct
	{
		__attribute__((aligned(8))) uintptr_t _this;
	} par = { _this };
	return Syscall(31, &par);
}

// Error api syscalls.

uint32_t GetLastError()
{
	return Syscall(32, NULL);
}
void SetLastError(uint32_t newError)
{
	Syscall(33, &newError);
}