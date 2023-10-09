#pragma once

#include <types.h>

typedef struct _key
{
	DWORD scanCode;
	SIZE_T nPressed;
	bool isPressed;
	char ch;
	char shiftAlias;
} key;

extern key g_keys[89];