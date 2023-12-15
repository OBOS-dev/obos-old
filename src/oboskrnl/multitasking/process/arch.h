/*
	oboskrnl/process/arch.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <multitasking/thread.h>

#if defined(__x86_64__) || defined(_WIN64)
#include <multitasking/process/x86_64/procInfo.h>
#endif

namespace obos
{
	namespace process
	{
		void setupContextInfo(procContextInfo* info);
		void switchToProcessContext(procContextInfo* info);
	}
}