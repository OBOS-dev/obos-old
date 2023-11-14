/*
	oboskrnl/error.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <error.h>

#include <multitasking/scheduler.h>
#include <multitasking/arch.h>
#include <multitasking/cpu_local.h>

#include <multitasking/process/process.h>

#define getCPULocal() ((thread::cpu_local*)thread::getCurrentCpuLocalPtr())

namespace obos
{
	void SetLastError(uint32_t err) 
	{
		getCPULocal()->currentThread->lastError = err;
#ifdef OBOS_DEBUG
		if (((process::Process*)getCPULocal()->currentThread->owner)->pid == 0)
			logger::warning("\nSetLastError called from a function called by the kernel, error code: %d.\n", err);
#endif
	}
	uint32_t GetLastError() 
	{
		return getCPULocal()->currentThread->lastError;
	}
}