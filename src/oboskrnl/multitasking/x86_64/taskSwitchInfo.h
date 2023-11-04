/*
	oboskrnl/multitasking/x86_64/taskSwitchInfo.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <arch/interrupt.h>

namespace obos
{
	namespace thread
	{
		struct taskSwitchInfo
		{
			void* cr3;
			void* tssStackBottom;
			interrupt_frame frame;
			alignas(0x10) uint8_t fpuState[512];
		};
	}
}