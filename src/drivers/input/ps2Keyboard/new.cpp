#include <types.h>

#include <driver_api/syscall_interface.h>

#include <stddef.h>

[[nodiscard]] PVOID operator new(size_t count) noexcept
{
	PVOID ret = nullptr;
	HeapAllocate(PASS_OBOS_API_PARS & ret, count);
	return ret;
}
[[nodiscard]] PVOID operator new[](size_t count) noexcept
{
	return operator new(count);
}
VOID operator delete(PVOID block) noexcept
{
	HeapFree(PASS_OBOS_API_PARS block);
}
VOID operator delete[](PVOID block) noexcept
{
	HeapFree(PASS_OBOS_API_PARS block);
}
VOID operator delete(PVOID block, size_t)
{
	HeapFree(PASS_OBOS_API_PARS block);
}
VOID operator delete[](PVOID block, size_t)
{
	HeapFree(PASS_OBOS_API_PARS block);
}