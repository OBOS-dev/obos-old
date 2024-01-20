/*
	arch/x86_64/syscall/verify_pars.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace syscalls
	{
		bool canAccessUserMemory(void* addr, size_t size, bool hasToWrite);
	}
}