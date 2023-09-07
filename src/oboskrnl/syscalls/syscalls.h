/*
	oboskrnl/syscalls/syscalls.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

extern "C" void isr64();

namespace obos
{
	namespace process
	{
		void RegisterSyscalls();
	}
}