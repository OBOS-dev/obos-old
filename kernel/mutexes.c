/*
	mutexes.c

	Copyright (c) 2023 Omar Berrow
*/

#include "mutexes.h"
#include "error.h"
#include "inline-asm.h"
#include "multitasking.h"

#include "liballoc/liballoc_1_1.h"

#include <stdatomic.h>

struct kMutexStructure
{
	// Whether the mutex was locked.
	ATOMIC_BOOL locked;
	// The process that owns the handle.
	DWORD owner;
	// The thread that owns the lock.
	DWORD lockOwner;
	BOOL isInitialized;
};
static struct kMutexStructure* s_kMutexHandles = NULLPTR;
static HANDLE s_nextMutexHandleValue = 0;

HANDLE MakeMutex()
{
	// While modifying kernel data structures, we don't want to be interrupted.
	EnterKernelSection();
	HANDLE ret = 0;
	if (!s_kMutexHandles && !s_nextMutexHandleValue)
		s_kMutexHandles = kcalloc(1, sizeof(struct kMutexStructure));
	else
	{
		for (int i = 0; i < s_nextMutexHandleValue; ret = i++)
			if (!s_kMutexHandles[i].isInitialized)
				break;
		if (!s_kMutexHandles[ret].isInitialized)
		{
			PVOID new_kMutexHandles = krealloc(s_kMutexHandles, sizeof(struct kMutexStructure) * (++s_nextMutexHandleValue));
			if (!new_kMutexHandles)
			{
				SetLastError(OBOS_ERROR_NO_MEMORY);
				sti();
				return OBOS_INVALID_HANDLE_VALUE;
			}
			ret = s_nextMutexHandleValue - 1;
			s_kMutexHandles = new_kMutexHandles;
		}
	}
	s_kMutexHandles[ret].isInitialized = TRUE;
	s_kMutexHandles[ret].locked = FALSE;
	s_kMutexHandles[ret].owner = /*GetPid()*/0;
	s_kMutexHandles[ret].lockOwner = (DWORD)-1;
	// We can now enable interrupts.
	LeaveKernelSection();
	return ret;
}

BOOL LockMutex(HANDLE hMutex)
{
	if (hMutex > s_nextMutexHandleValue)
	{
		SetLastError(OBOS_ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (!s_kMutexHandles[hMutex].isInitialized || s_kMutexHandles[hMutex].owner != /*GetPid()*/0)
	{
		SetLastError(OBOS_ERROR_INVALID_HANDLE);
		return FALSE;
	}
	while(atomic_load(&s_kMutexHandles[hMutex].locked) && s_kMutexHandles[hMutex].lockOwner != -1)
		__builtin_ia32_pause();
	atomic_store(&s_kMutexHandles[hMutex].locked, TRUE);
	s_kMutexHandles[hMutex].lockOwner = GetTid();
	return TRUE;
}

BOOL UnlockMutex(HANDLE hMutex)
{
	if (hMutex > s_nextMutexHandleValue)
	{
		SetLastError(OBOS_ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (!s_kMutexHandles[hMutex].isInitialized)
	{
		SetLastError(OBOS_ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (s_kMutexHandles[hMutex].lockOwner != GetTid())
	{
		SetLastError(OBOS_ERROR_ACCESS_DENIED);
		return FALSE;
	}
	atomic_store(&s_kMutexHandles[hMutex].locked, FALSE);
	s_kMutexHandles[hMutex].lockOwner = (DWORD)-1;
	return TRUE;
}
BOOL DestroyMutex(HANDLE hMutex)
{
	if (hMutex > s_nextMutexHandleValue)
	{
		SetLastError(OBOS_ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (!s_kMutexHandles[hMutex].isInitialized)
	{
		SetLastError(OBOS_ERROR_INVALID_HANDLE);
		return FALSE;
	}
	if (hMutex == s_nextMutexHandleValue - 1)
	{
		PVOID new_kMutexHandles = krealloc(s_kMutexHandles, sizeof(struct kMutexStructure) * (--s_nextMutexHandleValue));
		if (!new_kMutexHandles)
		{
			SetLastError(OBOS_ERROR_NO_MEMORY);
			return FALSE;
		}
		s_kMutexHandles = new_kMutexHandles;
		return TRUE;
	}
	EnterKernelSection();
	s_kMutexHandles[hMutex].isInitialized = FALSE;
	atomic_flag_clear(&s_kMutexHandles[hMutex].locked);
	s_kMutexHandles[hMutex].owner = 0;
	s_kMutexHandles[hMutex].lockOwner = 0;
	LeaveKernelSection();
	return TRUE;
}
