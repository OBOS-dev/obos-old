/*
	oboskrnl/error.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <error.h>

#include <multitasking/scheduler.h>

namespace obos
{
	void SetLastError(uint32_t err) 
	{
		thread::g_currentThread->lastError = err;
	}
	uint32_t GetLastError() 
	{
		return thread::g_currentThread->lastError;
	}
}