/*
	liballoc_hooks.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>
#include <inline-asm.h>

#include <memory_manager/paging/allocate.h>
#include <memory_manager/liballoc/liballoc.h>

#include <utils/memory.h>

extern "C" {
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

	void* liballoc_alloc(size_t nPages)
	{
		PVOID block = obos::memory::VirtualAlloc(0x00000000, nPages, obos::memory::VirtualAllocFlags::WRITE_ENABLED);
		if (block)
			obos::utils::memset(block, 0, nPages * 4096);
		return block;
	}
	int liballoc_free(void* block, size_t nPages)
	{
		return obos::memory::VirtualFree(block, nPages);
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
VOID operator delete(PVOID block, size_t) noexcept
{
	kfree(block);
}
VOID operator delete[](PVOID block, size_t) noexcept
{
	kfree(block);
}

// Placement new and delete.

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