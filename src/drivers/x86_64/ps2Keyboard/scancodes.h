/*
	drivers/x86_64/ps2Keyboard/scancodes.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <driverInterface/struct.h>

typedef struct _key
{
	uint32_t scanCode;
	size_t nPressed;
	bool isPressed;
	char ch;
	char shiftAlias;
	bool skipExtended = false;
	obos::driverInterface::SpecialKeys extendedCh; // Shift by 8 to get the SpecialKey enum version.
} key;

extern key g_keys[89];