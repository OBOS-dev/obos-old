/*
	oboskrnl/error.cpp

	Copyright (c) 2023-2024 Omar Berrow
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
		if (!getCPULocal())
			return;
		if (!getCPULocal()->currentThread)
			return;
		getCPULocal()->currentThread->lastError = err;
	}
	uint32_t GetLastError() 
	{
		return getCPULocal()->currentThread->lastError;
	}
}