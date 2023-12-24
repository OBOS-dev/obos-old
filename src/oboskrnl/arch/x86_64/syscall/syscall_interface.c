#include <int.h>

#include <arch/x86_64/syscall/syscall_interface.h>

#include <stdbool.h>

#ifdef __GNUC__
#define ALIGNAS(n) __attribute__((aligned(n)))
#else
#define ALIGNAS(n) 
#endif

extern uint64_t UMSyscall(uint16_t call, void* pars);
extern uint64_t KMSyscall(uint16_t call, void* pars);
extern uint16_t GetCS();
__asm__("UMSyscall: mov %rdi, %rax; mov %rsi, %rdi; syscall; ret");
__asm__("KMSyscall: mov %rdi, %rax; mov %rsi, %rdi; int $0x32; ret");
__asm__("GetCS: mov %cs, %rax; ret");

static uint64_t(*Syscall)(uint16_t call, void* pars);

#define INIT_MACRO \
if (!Syscall)\
{\
	uint16_t code_segment = GetCS();\
	Syscall = (code_segment == 0x08) ? KMSyscall : UMSyscall;\
}

// Virtual memory manager syscalls.
void* VirtualAlloc(void* base, size_t size, uintptr_t flags)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) void* base;
		ALIGNAS(8) size_t size;
		ALIGNAS(8) uintptr_t flags;
	} par = { base,size,flags };
	return (void*)Syscall(0, (void*)&par);
}
bool VirtualFree(void* base, size_t size)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) void* base;
		ALIGNAS(8) size_t size;
	} par = { base,size };
	return Syscall(1, (void*)&par);
}
bool VirtualProtect(void* base, size_t size, uintptr_t flags)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) void* base;
		ALIGNAS(8) size_t size;
		ALIGNAS(8) uintptr_t flags;
	} par = { base,size,flags };
	return Syscall(2, (void*)&par);
}
bool VirtualGetProtection(void* base, size_t size, uintptr_t* oFlags)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) void* base;
		ALIGNAS(8) size_t size;
		ALIGNAS(8) uintptr_t *oFlags;
	} par = { base,size,oFlags };
	Syscall(3, (void*)&par);
}

// Console manipulation syscalls.
void AllocConsole()
{
	INIT_MACRO;
	Syscall(4, NULL);
}
void ConsoleOutputCharacter(char ch)
{
	INIT_MACRO;
	Syscall(5, &ch);
}
void ConsoleOutputCharacterAt(char ch, uint32_t x, uint32_t y)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) char ch;
		ALIGNAS(8) uint32_t x;
		ALIGNAS(8) uint32_t y;
	} par = { ch,x,y };
	Syscall(6, &par);
}
void ConsoleOutputCharacterAtWithColour(char ch, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t x, uint32_t y)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) char ch;
		ALIGNAS(8) uint32_t foregroundColour;
		ALIGNAS(8) uint32_t backgroundColour;
		ALIGNAS(8) uint32_t x;
		ALIGNAS(8) uint32_t y;
	} par = { ch, foregroundColour,backgroundColour, x,y };
	Syscall(7, &par);
}
void ConsoleOutputString(const char* string)
{
	INIT_MACRO;
	Syscall(8, &string);
}
void ConsoleOutputStringSz(const char* string, size_t size)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) const char* string;
		ALIGNAS(8) size_t size;
	} par = { string,size };
	Syscall(9, &par);
}
void ConsoleSetPosition(uint32_t x, uint32_t y)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uint32_t x;
		ALIGNAS(8) uint32_t y;
	} par = { x,y };
	Syscall(10, &x);
}
void ConsoleGetPosition(uint32_t* x, uint32_t* y)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uint32_t* x;
		ALIGNAS(8) uint32_t* y;
	} par = { x,y };
	Syscall(11, &par);
}
void ConsoleSetColour(uint32_t foregroundColour, uint32_t backgroundColour, bool clearConsole)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uint32_t foregroundColour;
		ALIGNAS(8) uint32_t backgroundColour;
		ALIGNAS(8) bool clearConsole;
	} par = { foregroundColour, backgroundColour, clearConsole };
	Syscall(12, &par);
}
void ConsoleGetColour(uint32_t* foregroundColour, uint32_t* backgroundColour)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uint32_t *foregroundColour;
		ALIGNAS(8) uint32_t *backgroundColour;
	} par = { foregroundColour, backgroundColour };
	Syscall(13, &par);
}
void ConsoleSetFont(uint8_t* font)
{
	INIT_MACRO;
	Syscall(14, &font);
}
void ConsoleGetFont(uint8_t** oFont)
{
	INIT_MACRO;
	Syscall(15, &oFont);
}
void ConsoleSetFramebuffer(con_framebuffer framebuffer)
{
	INIT_MACRO;
	Syscall(16, &framebuffer);
}
void ConsoleGetFramebuffer(con_framebuffer* framebuffer)
{
	INIT_MACRO;
	Syscall(17, &framebuffer);
}
void ConsoleGetConsoleBounds(uint32_t* horizontal, uint32_t* vertical)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uint32_t* horizontal;
		ALIGNAS(8) uint32_t* vertical;
	} par = { horizontal, vertical };
	Syscall(18, &par);
}
void FreeConsole()
{
	INIT_MACRO;
	Syscall(19, NULL);
}

// Thread manipulation syscalls.

uintptr_t MakeThreadObject()
{
	INIT_MACRO;
	return Syscall(20, NULL);
}
bool CreateThread(uintptr_t _this, uint32_t priority, size_t stackSize, uint64_t affinity, void(*entry)(uintptr_t), uintptr_t userdata, bool startPaused)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uintptr_t _this; 
		ALIGNAS(8) uint32_t priority; 
		ALIGNAS(8) size_t stackSize;
		ALIGNAS(8) uint64_t affinity;
		ALIGNAS(8) void(*entry)(uintptr_t);
		ALIGNAS(8) uintptr_t userdata;
		ALIGNAS(8) bool startPaused;
	} par = { _this,priority,stackSize,affinity,entry,userdata,startPaused };
	return Syscall(21, &par);
}
bool OpenThread(uintptr_t _this, uint32_t tid)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uintptr_t _this;
		ALIGNAS(8) uint32_t tid;
	} par = { _this,tid };
	return Syscall(22, &par);
}
bool PauseThread(uintptr_t _this)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uintptr_t _this;
	} par = { _this };
	return Syscall(23, &par);
}
bool ResumeThread(uintptr_t _this)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uintptr_t _this;
	} par = { _this };
	return Syscall(24, &par);
}
bool SetThreadPriority(uintptr_t _this, uint32_t priority)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uintptr_t _this;
		ALIGNAS(8) uint32_t priority;
	} par = { _this,priority };
	return Syscall(25, &par);
}
bool TerminateThread(uintptr_t _this)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uintptr_t _this;
	} par = { _this };
	return Syscall(26, &par);
}
uint32_t GetThreadStatus(uintptr_t _this)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uintptr_t _this;
	} par = { _this };
	return Syscall(27, &par);
}
uint32_t GetThreadExitCode(uintptr_t _this)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uintptr_t _this;
	} par = { _this };
	return Syscall(28, &par);
}
uint32_t GetThreadLastError(uintptr_t _this)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uintptr_t _this;
	} par = { _this };
	return Syscall(29, &par);
}
bool CloseThread(uintptr_t _this)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uintptr_t _this;
	} par = { _this };
	return Syscall(30, &par);
}
bool FreeThread(uintptr_t _this)
{
	INIT_MACRO;
	struct
	{
		ALIGNAS(8) uintptr_t _this;
	} par = { _this };
	return Syscall(31, &par);
}

// Error api syscalls.

uint32_t GetLastError()
{
	INIT_MACRO;
	return Syscall(32, NULL);
}
void SetLastError(uint32_t newError)
{
	INIT_MACRO;
	Syscall(33, &newError);
}