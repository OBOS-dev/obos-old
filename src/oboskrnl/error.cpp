/*
	oboskrnl/error.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>
#include <error.h>

#include <multitasking/multitasking.h>

namespace obos
{
	static DWORD s_error = 0;
	DWORD GetLastError()
	{
		if (multitasking::g_initialized)
			return multitasking::g_currentThread->lastError;
		else
			return s_error;
	}
	void SetLastError(DWORD err)
	{
		if (multitasking::g_initialized)
			multitasking::g_currentThread->lastError = err;
		else
			s_error = err;
	}
}