/*
	liballoc_hooks.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>
#include <inline-asm.h>

#include <memory_manager/paging/allocate.h>
#include <memory_manager/liballoc/liballoc.h>

#include <utils/memory.h>

#include <stddef.h>

namespace obos
{
	SIZE_T g_nLiballocPagesAllocated = 0;
}

extern "C"
{
	void* liballoc_alloc(size_t nPages)
	{
		PVOID block = obos::memory::VirtualAlloc(reinterpret_cast<PVOID>(0xD0000000 + (obos::g_nLiballocPagesAllocated << 12)), nPages, obos::memory::VirtualAllocFlags::WRITE_ENABLED);
		obos::g_nLiballocPagesAllocated += nPages;
		if (block)
			obos::utils::memzero(block, nPages << 12);
		return block;
	}
	int liballoc_free(void* block, size_t nPages)
	{
		obos::g_nLiballocPagesAllocated -= nPages;
		return obos::memory::VirtualFree(block, nPages);
	}
	
	int liballoc_lock()
	{
		obos::EnterKernelSection();
		return 0;
	}
	
	int liballoc_unlock()
	{
		obos::LeaveKernelSection();
		return 0;
	}
}

// (De)allocating new and delete.

[[nodiscard]] PVOID operator new(size_t count) noexcept
{
	return kcalloc(count, sizeof(char));
}
[[nodiscard]] PVOID operator new[](size_t count) noexcept
{
	return kcalloc(count, sizeof(char));
}
VOID operator delete(PVOID block) noexcept
{
	kfree(block);
}
VOID operator delete[](PVOID block) noexcept
{
	kfree(block);
}
VOID operator delete(PVOID block, size_t)
{
	kfree(block);
}
VOID operator delete[](PVOID block, size_t)
{
	kfree(block);
}

[[nodiscard]] PVOID operator new(size_t, void* ptr) noexcept
{
	return ptr;
}
[[nodiscard]] PVOID operator new[](size_t, void* ptr) noexcept
{
	return ptr;
}
VOID operator delete(PVOID, PVOID) noexcept
{}
VOID operator delete[](PVOID, PVOID) noexcept
{}