/*
	driverInterface/x86_64/call.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <limine.h>

#include <int.h>
#include <klog.h>
#include <memory_manipulation.h>

#include <driverInterface/call.h>

#include <arch/x86_64/memory_manager/virtual/allocate.h>

namespace obos
{
	extern volatile limine_module_request module_request;
	namespace driverInterface
	{
		uintptr_t g_syscallTable[256];

		uintptr_t RegisterSyscall(byte n, uintptr_t addr)
		{
			uintptr_t ret = g_syscallTable[n];
			g_syscallTable[n] = addr;
			return ret;
		}

		size_t SyscallVPrintf(void* pars)
		{
			struct _par
			{
				const char* format;
				va_list list;
			} *par = (_par*)pars;
			return logger::vprintf(par->format, par->list);
		}

		void* SyscallMalloc(size_t* size)
		{
			return new char[*size];
		}
		void SyscallFree(void** block)
		{
			delete[] (char*)*block;
		}
		void* SyscallMapPhysToVirt(void** pars)
		{
			void* virt = pars[0];
			void* phys = pars[1];
			uintptr_t protFlags = (uintptr_t)pars[2];
			memory::MapVirtualPageToPhysical(virt,phys, memory::DecodeProtectionFlags(protFlags));
			return virt;
		}
		void* SyscallGetInitrdLocation(size_t** oSize)
		{
			void* ret = nullptr;

			size_t moduleIndex = 0;

			for (; moduleIndex < module_request.response->module_count; moduleIndex++)
			{
				if (utils::strcmp(module_request.response->modules[moduleIndex]->path, "/obos/initrd.tar"))
				{
					ret = module_request.response->modules[moduleIndex]->address;
					break;
				}
			}
			if (ret)
				**oSize = module_request.response->modules[moduleIndex]->size;
			return ret;
		}
	}
}