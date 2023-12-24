/*
	oboskrnl/arch/x86_64/stack_canary.cpp

	Copyright (c) 2023 Omar Berrow
*/
#ifdef OBOS_DEBUG
#include <int.h>
#include <klog.h>

#include <allocators/vmm/vmm.h>

#include <x86_64-utils/asm.h>

uintptr_t __stack_chk_guard = 0x15E4F6DABD6F8A;

struct frame
{
	frame* next;
	void *rip;
};
extern "C" [[noreturn]] void __stack_chk_fail(void)
{
	frame* current = (frame*)obos::getRBP();
	void* where = nullptr;
	// Why did I go through all this JUST to get the return address
	{
		if (!current)
			goto skip;
		uint64_t attrib = 0;
		size_t nPages = 0;
		uintptr_t _current = (uintptr_t)current;
		nPages = 1llu + (((_current + sizeof(*current)) & ~0xfff) > (_current & ~0xfff));
		obos::memory::VirtualAllocator vallocator{ nullptr };
		if (!vallocator.VirtualGetProtection(current, nPages, &attrib))
			goto skip;
		if (!(attrib & obos::memory::PROT_IS_PRESENT))
			goto skip;
		where = current->rip;
	}
	skip:
	obos::logger::panic(nullptr, "Stack corruption detected at %p.\n", where);
}
#endif