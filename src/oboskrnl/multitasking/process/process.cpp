/*
	oboskrnl/process/process.h

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>

#include <multitasking/thread.h>
#include <multitasking/scheduler.h>

#include <multitasking/process/process.h>
#include <multitasking/process/arch.h>

namespace obos
{
	namespace process
	{
		uint32_t g_nextPID = 1;
		Process* CreateProcess(bool isUsermode)
		{
			Process* ret = new Process{};

			setupContextInfo(&ret->context);
			ret->pid = g_nextPID++;
			ret->isUsermode = isUsermode;
			ret->parent = (process::Process*)thread::g_currentThread->owner;

			return ret;
		}
	}
}