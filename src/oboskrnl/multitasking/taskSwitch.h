/*
	oboskrnl/multitasking/taskSwitch.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <arch/interrupt.h>

namespace obos
{
	namespace thread
	{
#if defined(__x86_64__)
#include <multitasking/x86_64/taskSwitchInfo.h>
#endif
		void switchToThreadImpl(taskSwitchInfo* info);
	}
}