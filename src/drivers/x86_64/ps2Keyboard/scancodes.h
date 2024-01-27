/*
	drivers/x86_64/ps2Keyboard/scancodes.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

typedef struct _key
{
	uint32_t scanCode;
	size_t nPressed;
	bool isPressed;
	char ch;
	char shiftAlias;
} key;

extern key g_keys[89];