#pragma once

#include <int.h>

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#include <stdbool.h>
#define EXTERN_C
#endif

// Virtual memory manager syscalls.
EXTERN_C void* VirtualAlloc(void* base, size_t size, uintptr_t flags);
EXTERN_C bool VirtualFree(void* base, size_t size);
EXTERN_C bool VirtualProtect(void* base, size_t size, uintptr_t flags);
EXTERN_C bool VirtualGetProtection(void* base, size_t size, uintptr_t* oFlags);

// Console manipulation syscalls.
EXTERN_C void AllocConsole();
EXTERN_C void ConsoleOutputCharacter(char ch);
EXTERN_C void ConsoleOutputCharacterAt(char ch, uint32_t x, uint32_t y);
EXTERN_C void ConsoleOutputCharacterAtWithColour(char ch, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t x, uint32_t y);
EXTERN_C void ConsoleOutputString(const char* string);
EXTERN_C void ConsoleOutputStringSz(const char* string, size_t size);
EXTERN_C void ConsoleSetPosition(uint32_t x, uint32_t y);
EXTERN_C void ConsoleGetPosition(uint32_t* x, uint32_t* y);
EXTERN_C void ConsoleSetColour(uint32_t foregroundColour, uint32_t backgroundColour, bool clearConsole);
EXTERN_C void ConsoleGetColour(uint32_t* foregroundColour, uint32_t* backgroundColour);
EXTERN_C void ConsoleSetFont(uint8_t* font);
EXTERN_C void ConsoleGetFont(uint8_t** oFont);
typedef struct _con_framebuffer
{
	uint32_t* addr;
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
} con_framebuffer;
EXTERN_C void ConsoleSetFramebuffer(con_framebuffer framebuffer);
EXTERN_C void ConsoleGetFramebuffer(con_framebuffer* framebuffer);
EXTERN_C void ConsoleGetConsoleBounds(uint32_t* horizontal, uint32_t* vertical);
EXTERN_C void FreeConsole();
EXTERN_C uintptr_t MakeThreadObject();
EXTERN_C bool CreateThread(uintptr_t _this, uint32_t priority, size_t stackSize, uint64_t affinity, void(*entry)(uintptr_t), uintptr_t userdata, bool startPaused);
EXTERN_C bool OpenThread(uintptr_t _this, uint32_t tid);
EXTERN_C bool PauseThread(uintptr_t _this);
EXTERN_C bool ResumeThread(uintptr_t _this);
EXTERN_C bool SetThreadPriority(uintptr_t _this, uint32_t priority);
EXTERN_C bool TerminateThread(uintptr_t _this);
EXTERN_C uint32_t GetThreadStatus(uintptr_t _this);
EXTERN_C uint32_t GetThreadExitCode(uintptr_t _this);
EXTERN_C uint32_t GetThreadLastError(uintptr_t _this);
EXTERN_C bool CloseThread(uintptr_t _this);
EXTERN_C bool FreeThread(uintptr_t _this);
EXTERN_C uint32_t GetLastError();
EXTERN_C void SetLastError(uint32_t newError);