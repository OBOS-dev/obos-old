/*
	arch/x86_64/syscall/console_syscalls.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <console.h>
#include <klog.h>
#include <error.h>

#include <arch/x86_64/memory_manager/virtual/allocate.h>

#include <arch/x86_64/syscall/verify_pars.h>
#include <arch/x86_64/syscall/console_syscalls.h>

#include <multitasking/scheduler.h>
#include <multitasking/arch.h>
#include <multitasking/cpu_local.h>

#include <multitasking/process/process.h>

#define getCPULocal() ((thread::cpu_local*)thread::getCurrentCpuLocalPtr())

namespace obos
{
	namespace syscalls
	{
		void SyscallAllocConsole()
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			if (!proc->console)
				proc->console = new Console;
			void* font = nullptr;
			con_framebuffer framebuffer;
			uint32_t initialY = 0;
			if (proc->parent->console)
			{
				proc->parent->console->GetFont((uint8_t**)&font);
				proc->parent->console->GetFramebuffer(&framebuffer);
				proc->parent->console->GetPosition(nullptr, &initialY);
			}
			else
			{
				g_kernelConsole.GetFont((uint8_t**)&font);
				g_kernelConsole.GetFramebuffer(&framebuffer);
				g_kernelConsole.GetPosition(nullptr, &initialY);
			}
			proc->console->Initialize(font, framebuffer);
			proc->console->SetPosition(0, initialY + 1);
			proc->console->SetColour(0xffffffff, 0);
		}

		void SyscallConsoleOutputCharacter(void* pars)
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			char* ch = (char*)pars;
			if (!canAccessUserMemory(ch, sizeof(*ch), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (proc->console)
				proc->console->ConsoleOutput(*ch);
		}
		void SyscallConsoleOutputCharacterAt(void* _pars)
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			struct _par
			{
				alignas(0x08) char ch;
				alignas(0x08) uint32_t x;
				alignas(0x08) uint32_t y;
			} *pars = (_par*)_pars;
			if (!canAccessUserMemory(pars, sizeof(*pars), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (proc->console)
				proc->console->ConsoleOutput(pars->ch, pars->x, pars->y);
		}
		void SyscallConsoleOutputCharacterAtWithColour(void* _pars)
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			struct _par
			{
				alignas(0x08) char ch;
				alignas(0x08) uint32_t foregroundColour;
				alignas(0x08) uint32_t backgroundColour;
				alignas(0x08) uint32_t x;
				alignas(0x08) uint32_t y;
			} *pars = (_par*)_pars;
			if (!canAccessUserMemory(pars, sizeof(*pars), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (proc->console)
				proc->console->ConsoleOutput(pars->ch, pars->foregroundColour, pars->backgroundColour, pars->x, pars->y);
		}
		void SyscallConsoleOutputString(void* pars)
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			const char** str = (const char**)pars;
			if (!canAccessUserMemory(str, sizeof(str), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (!canAccessUserMemory((void*)*str, sizeof(*str), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (proc->console)
				proc->console->ConsoleOutput(*str);
		}
		void SyscallConsoleOutputStringSz(void* _pars)
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			struct _par
			{
				alignas(0x08) const char* str;
				alignas(0x08) size_t size;
			} *pars = (_par*)_pars;
			if (!canAccessUserMemory(pars, sizeof(*pars), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (proc->console)
				proc->console->ConsoleOutput(pars->str, pars->size);
		}
		void SyscallConsoleSetPosition(uint32_t* pars)
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			if (!canAccessUserMemory(pars, sizeof(*pars) * 2, false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (proc->console)
				proc->console->SetPosition(pars[0], pars[2]);
		}
		void SyscallConsoleGetPosition(uint32_t** pars)
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			if (!canAccessUserMemory(pars, sizeof(*pars) * 2, false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (!canAccessUserMemory(pars[0], sizeof(*pars), true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (!canAccessUserMemory(pars[1], sizeof(*pars), true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (proc->console)
				proc->console->GetPosition(pars[0], pars[1]);
		}
		void SyscallConsoleSetColour(uint32_t* pars)
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			if (!canAccessUserMemory(pars, sizeof(*pars) * 3, false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (proc->console)
				proc->console->SetColour(pars[0], pars[2], pars[4]);
		}
		void SyscallConsoleGetColour(uint32_t** pars)
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			if (!canAccessUserMemory(pars, sizeof(*pars) * 2, false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (!canAccessUserMemory(pars[0], sizeof(*pars), true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (!canAccessUserMemory(pars[1], sizeof(*pars), true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (proc->console)
				proc->console->GetColour(pars[0], pars[1]);
		}
		void SyscallConsoleSetFont(uint8_t** font)
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			if (!canAccessUserMemory(font, sizeof(*font), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (!canAccessUserMemory(*font, 4096, false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (proc->console)
				proc->console->SetFont(*font);
		}
		void SyscallConsoleGetFont(uint8_t*** font)
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			if (!canAccessUserMemory(font, sizeof(*font), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (!canAccessUserMemory(*font, sizeof(**font), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (!canAccessUserMemory(**font, 4096, true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			uint8_t* _font = nullptr;
			if (proc->console)
				proc->console->GetFont(&_font);
			uint8_t* ret = (uint8_t*)memory::VirtualAlloc(nullptr, 1, memory::PROT_USER_MODE_ACCESS);
			**font = ret;
			for (int i = 0; i < 4096; i++)
				ret[i] = _font[i];
		}
		void SyscallConsoleSetFramebuffer(void* _pars)
		{
			con_framebuffer* pars = (con_framebuffer*)_pars;
			if (!canAccessUserMemory(pars, sizeof(*pars), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			if (proc->console)
				proc->console->SetFramebuffer(*pars);
		}
		void SyscallConsoleGetFramebuffer(void* _pars)
		{
			con_framebuffer** pars = (con_framebuffer**)_pars;
			if (!canAccessUserMemory(pars, sizeof(*pars), false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (!canAccessUserMemory(*pars, sizeof(*pars), true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			if (proc->console)
				proc->console->GetFramebuffer(*pars);
		}
		void SyscallConsoleGetConsoleBounds(uint32_t** pars)
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			if (!canAccessUserMemory(pars, sizeof(*pars) * 2, false))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (!canAccessUserMemory(pars[0], sizeof(*pars), true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (!canAccessUserMemory(pars[1], sizeof(*pars), true))
			{
				SetLastError(OBOS_ERROR_INVALID_PARAMETER);
				return;
			}
			if (proc->console)
				proc->console->GetConsoleBounds(pars[0], pars[1]);
		}

		void SyscallFreeConsole()
		{
			process::Process* proc = (process::Process*)getCPULocal()->currentThread->owner;
			if (proc->console)
			{
				delete proc->console;
				proc->console = nullptr;
			}
		}
	}
}