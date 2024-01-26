/*
	arch/x86_64/syscall/sconsole.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <error.h>
#include <console.h>

#include <arch/x86_64/syscall/handle.h>
#include <arch/x86_64/syscall/console.h>
#include <arch/x86_64/syscall/verify_pars.h>

#include <multitasking/cpu_local.h>

#include <multitasking/process/process.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t ConsoleSyscallHandler(uint64_t syscall, void* args)
		{
			void (*v_pu32_pu32)(uint32_t* p1, uint32_t *p2) = nullptr;
			switch (syscall)
			{
			case 25:
				return SyscallInitializeProcessConsole();
			case 26:
			{
				struct _par
				{
					alignas(0x10) const char* string;
					alignas(0x10) size_t size;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
					return 0;
				if (!canAccessUserMemory(pars->string, pars->size, false))
					return 0;
				SyscallConsoleOutput(pars->string, pars->size);
				break;
			}
			case 27:
			{
				struct _par
				{
					char ch;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
					return 0;
				SyscallConsoleOutput(pars->ch);
				break;
			}
			case 28:
			{
				struct _par
				{
					alignas(0x10) char ch;
					alignas(0x10) uint32_t x; 
					alignas(0x10) uint32_t y;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
					return 0;
				SyscallConsoleOutput(pars->ch, pars->x, pars->y);
				break;
			}
			case 29:
			{
				struct _par
				{
					alignas(0x10) char ch;
					alignas(0x10) uint32_t foregroundColour;
					alignas(0x10) uint32_t backgroundColour;
					alignas(0x10) uint32_t x; 
					alignas(0x10) uint32_t y;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
					return 0;
				SyscallConsoleOutput(pars->ch, pars->foregroundColour, pars->backgroundColour, pars->x, pars->y);
				break;
			}
			case 30:
			{
				struct _par
				{
					alignas(0x10) uint32_t par1;
					alignas(0x10) uint32_t par2;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
					return 0;
				SyscallSetPosition(pars->par1, pars->par2);
				break;
			}
			case 31:
				v_pu32_pu32 = SyscallGetPosition;
				goto _v_pu32_pu32;
			case 36:
				v_pu32_pu32 = SyscallGetConsoleBounds;
				goto _v_pu32_pu32;
			case 33:
				v_pu32_pu32 = SyscallGetColour;
			{
				_v_pu32_pu32:
				struct _par
				{
					alignas(0x10) uint32_t *par1;
					alignas(0x10) uint32_t *par2;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
					return 0;
				if ((pars->par1 && !canAccessUserMemory(pars->par1, sizeof(*pars->par1), true)) ||
					(pars->par2 && !canAccessUserMemory(pars->par1, sizeof(*pars->par2), true)))
					return 0;
				v_pu32_pu32(pars->par1, pars->par2);
				break;
			}
			case 32:
			{
				struct _par
				{
					alignas(0x10) uint32_t par1;
					alignas(0x10) uint32_t par2;
					alignas(0x10) bool clearConsole;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
					return 0;
				SyscallSetColour(pars->par1, pars->par2, pars->clearConsole);
				break;
			}
			case 34:
			{
				struct _par
				{
					alignas(0x10) uint8_t* font;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
					return 0;
				SyscallSetFont(pars->font);
				break;
			}
			case 35:
			{
				struct _par
				{
					alignas(0x10) uint8_t** font;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
					return 0;
				SyscallGetFont(pars->font);
				break;
			}
			case 37:
				SyscallSwapBuffers();
				break;
			case 38:
			{
				struct _par
				{
					alignas(0x10) bool isBackbuffer;
				} *pars = (_par*)args;
				if (!canAccessUserMemory(pars, sizeof(*pars), false))
					return 0;
				SyscallSetDrawBuffer(pars->isBackbuffer);
				break;
			}
			default:
				break;
			}
			return 0;
		}

		static Console& GetProcessConsole(process::Process* proc)
		{
			if (!proc)
				proc = (process::Process*)thread::GetCurrentCpuLocalPtr()->currentThread->owner;
			return *proc->console;
		}
		static void SetProcessConsole(process::Process* proc, Console* newConsole)
		{
			if (!proc)
				proc = (process::Process*)thread::GetCurrentCpuLocalPtr()->currentThread->owner;
			proc->console = newConsole;
		}

		
		bool SyscallInitializeProcessConsole()
		{ 
			if (&GetProcessConsole(nullptr) != &g_kernelConsole)
			{
				SetLastError(OBOS_ERROR_ALREADY_EXISTS);
				return false;
			}
			con_framebuffer fbuffer{};
			g_kernelConsole.GetFramebuffer(&fbuffer);
			uint8_t* font = nullptr;
			g_kernelConsole.GetFont(&font);
			Console* con = new Console;
			con->Initialize(font, fbuffer, true);
			SetProcessConsole(nullptr, con);
			return true; 
		}
		void SyscallConsoleOutput(const char* string, size_t size)
		{
			if (!canAccessUserMemory(string, size, false))
				return;
			GetProcessConsole(nullptr).ConsoleOutput(string, size);
		}
		void SyscallConsoleOutput(char ch)
		{
			GetProcessConsole(nullptr).ConsoleOutput(ch);

		}
		void SyscallConsoleOutput(char ch, uint32_t x, uint32_t y)
		{
			GetProcessConsole(nullptr).ConsoleOutput(ch, x,y);
		}
		void SyscallConsoleOutput(char ch, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t x, uint32_t y) 
		{
			GetProcessConsole(nullptr).ConsoleOutput(ch, foregroundColour,backgroundColour, x,y);
		}

		void SyscallSetPosition(uint32_t x, uint32_t y)
		{
			GetProcessConsole(nullptr).SetPosition(x,y);
		}
		void SyscallGetPosition(uint32_t* x, uint32_t* y)
		{
			// The pointers to x and y are validated by ConsoleSyscallHandler.
			GetProcessConsole(nullptr).GetPosition(x, y);
		}
		void SyscallSetColour(uint32_t foregroundColour, uint32_t backgroundColour, bool clearConsole)
		{
			GetProcessConsole(nullptr).SetColour(foregroundColour, backgroundColour, clearConsole);
		}
		void SyscallGetColour(uint32_t* foregroundColour, uint32_t* backgroundColour)
		{
			// The pointers to foregroundColour and backgroundColour are validated by ConsoleSyscallHandler.
			GetProcessConsole(nullptr).GetColour(foregroundColour, backgroundColour);
		}
		void SyscallSetFont(uint8_t* font)
		{
			if (!font)
				return;
			if (&GetProcessConsole(nullptr) == &g_kernelConsole)
				return;
			if (!canAccessUserMemory(font, 4096, false))
				return;
			uint8_t* kFont = (uint8_t*)memory::VirtualAllocator{ nullptr }.VirtualAlloc(nullptr, 4096, 0);
			utils::memcpy(kFont, font, 4096);
			GetProcessConsole(nullptr).SetFont(kFont);
		}
		void SyscallGetFont(uint8_t** font)
		{
			if (!font)
				return;
			if (!canAccessUserMemory(font, sizeof(*font), false))
				return;
			uint8_t* uFont = (uint8_t*)memory::VirtualAllocator{ nullptr }.VirtualAlloc(nullptr, 4096, memory::PROT_USER_MODE_ACCESS);
			uint8_t* kFont = nullptr;
			GetProcessConsole(nullptr).GetFont(&kFont);
			utils::memcpy(uFont, kFont, 4096);
			*font = uFont;
		}
		void SyscallGetConsoleBounds(uint32_t* horizontal, uint32_t* vertical)
		{
			GetProcessConsole(nullptr).GetConsoleBounds(horizontal, vertical);
		}

		void SyscallSwapBuffers()
		{
			GetProcessConsole(nullptr).SwapBuffers();
		}
		void SyscallSetDrawBuffer(bool isBackbuffer)
		{
			if (&GetProcessConsole(nullptr) == &g_kernelConsole)
				return;
			GetProcessConsole(nullptr).SetDrawBuffer(isBackbuffer);
		}
	}
}