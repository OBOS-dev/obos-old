#include <int.h>

#include <driverInterface/interface.h>
#include <driverInterface/x86_64/call_interface.h>

#include <stdarg.h>

extern "C" uintptr_t Syscall(uintptr_t call, void* callPars);

namespace obos
{
	size_t VPrintf(const char* str, va_list list)
	{
		struct
		{
			const char* str;
			va_list list;
		} pars = { .str = str, .list = *list };
		return Syscall(0, &pars);
	}
	void ExitThread(uint32_t exitCode)
	{
		Syscall(1, &exitCode);
	}
	driverInterface::DriverServer* AllocDriverServer()
	{
		return (driverInterface::DriverServer*)Syscall(2, nullptr);
	}
	void FreeDriverServer(driverInterface::DriverServer* obj)
	{
		Syscall(3, &obj);
	}
	void* Malloc(size_t size)
	{
		return (void*)Syscall(4, &size);
	}
	void Free(void* blk)
	{
		Syscall(5, &blk);
	}
	void* MapPhysToVirt(void* virt, void* phys, uintptr_t protFlags)
	{
		struct
		{
			void* virt, *phys;
			uintptr_t protFlags;
		} pars = { .virt=virt, .phys=phys, .protFlags=protFlags };
		return (void*)Syscall(6, &pars);
	}
	void* GetInitrdLocation(size_t* size)
	{
		return (void*)Syscall(7, &size);
	}
}

asm("Syscall: mov %rdi, %rax; mov %rsi, %rdi; int $0x31; ret");