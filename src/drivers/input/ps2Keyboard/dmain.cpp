/*
	drivers/input/ps2Keyboard/dmain.c
	
	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>

#include <driver_api/enums.h>
#include <driver_api/syscall_interface.h>

#include <syscalls/syscall_interface.h>

#include <stdarg.h>

#include "scancodes.h"
#include "inline-asm.h"

#define DRIVER_ID 0
#define ACK 0xfa

void keyboardInterrupt();

BYTE sendCommand(DWORD nCommands, ...);

// Should take 2 pages.
BYTE g_keyBuffer[8192] = {};
SIZE_T g_keyBufferPosition = 0;

struct
{
	bool isScrollLock : 1;
	bool isNumberLock : 1;
	bool isCapsLock : 1;
	bool isShiftPressed : 1;
} attribute(packed) flags = {0,0,0,0};

obos::driverAPI::driverHeader __attribute__((section(OBOS_DRIVER_HEADER_SECTION))) g_driverHeader = {
	.magicNumber = OBOS_DRIVER_HEADER_MAGIC,
	.driverId = DRIVER_ID,
	.service_type = obos::driverAPI::serviceType::OBOS_SERVICE_TYPE_USER_INPUT_DEVICE,
	.infoFields = 0,
	.pciInfo = {}
};

void ConnectionHandler(PVOID);

extern "C" int _start()
{
	DisableIRQ(PASS_OBOS_API_PARS 1);

	// Keys need to held for 250 ms before repeating, and they repeat at a rate of 30 hz (33.33333 ms).
	sendCommand(2, 0xF3, 0);

	// Set scancode set 1 (scancode set 2?).
	sendCommand(2, 0xF0, 2);

	// Enable scanning.
	sendCommand(1, 0xF4);

	sendCommand(2, 0xED, 0b111);
	
	RegisterInterruptHandler(PASS_OBOS_API_PARS 0x21, keyboardInterrupt);

	EnableIRQ(PASS_OBOS_API_PARS 1);
	
	HANDLE connection = 0;
	HANDLE thread = 0;

	while (1)
	{
		if (ListenForConnections(PASS_OBOS_API_PARS (&connection)) != obos::driverAPI::exitStatus::EXIT_STATUS_SUCCESS)
			return GetLastError();

		CreateThread(PASS_OBOS_API_PARS &thread, 4, ConnectionHandler, (PVOID)connection, 0, 0);
		CloseHandle(PASS_OBOS_API_PARS thread);
	}

	return 0;
}

void keyboardInterrupt()
{
	BYTE scancode = inb(0x60);

	bool wasReleased = (scancode & 0x80) == 0x80;
	
	char ch = 0;
	
	if (scancode > 0xD8)
		return;

	if (wasReleased)
		scancode &= ~(0x80);

	ch = g_keys[scancode].ch;

	g_keys[scancode].isPressed = !wasReleased;
	g_keys[scancode].nPressed++;
	if(g_keys[scancode].ch && g_keys[scancode].isPressed)
	{
		bool isUppercase = flags.isCapsLock || flags.isShiftPressed;
		if (flags.isCapsLock && flags.isShiftPressed)
			isUppercase = false;

		char newKey = 0;
		if(ch < 'A' || ch > 'Z')
			newKey = ch;
		else
			newKey = isUppercase ? ch : (ch - 'A') + 'a';
		if (flags.isShiftPressed && (ch < 'A' || ch > 'Z') && g_keys[scancode].shiftAlias)
			newKey = g_keys[scancode].shiftAlias;
		if(newKey)
			g_keyBuffer[g_keyBufferPosition++] = newKey;
		/*if(newKey != '\n')
		{
			outb(0x3f8, newKey);
			PrintChar(PASS_OBOS_API_PARS newKey, true);
		}
		else
		{
			PrintChar(PASS_OBOS_API_PARS '\r', false);
			PrintChar(PASS_OBOS_API_PARS '\n', true);
			outb(0x3f8, '\r');
			outb(0x3f8, '\n');
		}
		if (newKey == '\b')
		{
			outb(0x3f8, ' ');
			outb(0x3f8, '\b');
		}*/
	}

	if (scancode == 0x2A || scancode == 0x36)
		flags.isShiftPressed = !wasReleased;

	if (wasReleased)
	{
		g_keys[scancode].nPressed = 0;
		return;
	}


	switch (scancode)
	{
	case 0x3A:
	{
		flags.isCapsLock = !flags.isCapsLock;
		BYTE bitfield = flags.isScrollLock | (flags.isNumberLock << 1) | (flags.isCapsLock << 2);
		sendCommand(2, 0xED, bitfield);
		break;
	}
	case 0x46:
	{
		flags.isScrollLock = !flags.isScrollLock;
		BYTE bitfield = flags.isScrollLock | (flags.isNumberLock << 1) | (flags.isCapsLock << 2);
		sendCommand(2, 0xED, bitfield);
		break;
	}
	case 0x45:
	{
		flags.isNumberLock = !flags.isNumberLock;
		BYTE bitfield = flags.isScrollLock | (flags.isNumberLock << 1) | (flags.isCapsLock << 2);
		sendCommand(2, 0xED, bitfield);
		break;
	}
	default:
		break;
	}
}

BYTE sendCommand(DWORD nCommands, ...)
{
	while ((inb(0x64) & 0b10) == 0b10);
	va_list list;
	va_start(list, nCommands);
	BYTE ret = 0x00;
	for (int i = 0; i < 5; i++)
	{
		for (DWORD x = 0; x < nCommands; x++)
			outb(0x60, va_arg(list, int));
		ret = inb(0x60);
		if (ret == ACK)
			break;
	}
	va_end(list);
	return ret;
}