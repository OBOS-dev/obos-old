/*
	oboskrnl/arch/x86_64/gdbstub/stub.h

	Copyright (c) 2023 Omar Berrow	
*/

#pragma once

#include <int.h>

#include <arch/x86_64/gdbstub/communicate.h>

namespace obos
{
	namespace gdbstub
	{
		extern bool g_stubInitialized;
		void InititalizeGDBStub(Connection* conn);
	}
}