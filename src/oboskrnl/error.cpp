/*
	oboskrnl/error.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <error.h>

#include <multitasking/scheduler.h>

#include <multitasking/process/process.h>

namespace obos
{
	void SetLastError(uint32_t err) 
	{
		thread::g_currentThread->lastError = err;
#ifdef OBOS_DEBUG
		if (((process::Process*)thread::g_currentThread->owner)->pid == 0)
			logger::warning("\nSetLastError called from a function called by the kernel, error code: %d.\n", err);
#endif
	}
	uint32_t GetLastError() 
	{
		return thread::g_currentThread->lastError;
	}
}