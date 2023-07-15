/*
	error.c

	Copyright (c) 2023 Omar Berrow
*/

#include "error.h"

DWORD g_kErrorCode = 0;

DWORD GetLastError()
{ return g_kErrorCode; }
void SetLastError(DWORD errCode)
{ g_kErrorCode = errCode; }