/*
	drivers/x86_64/ps2Keyboard/dmain.cpp
	
	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <utils/vector.h>
#include <console.h>

#include <new>

#include <x86_64-utils/asm.h>

#include <driverInterface/struct.h>

#include <stdarg.h>

#include <arch/x86_64/irq/irq.h>
#include <arch/interrupt.h>

#include "scancodes.h"

#define ACK 0xfa
#define RESEND 0xfe

using namespace obos;

static void keyboardInterrupt(interrupt_frame*);

byte sendCommand(uint32_t nCommands, ...);

utils::Vector<uint16_t> g_keyBuffer;

struct
{
	bool isScrollLock : 1;
	bool isNumberLock : 1;
	bool isCapsLock : 1;
	bool isShiftPressed : 1;
} flags = { 0,0,0,0 };

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
__attribute__((section(OBOS_DRIVER_HEADER_SECTION_NAME))) driverInterface::driverHeader g_driverHeader = {
	.magicNumber = driverInterface::OBOS_DRIVER_HEADER_MAGIC,
	.driverId = 5,
	.driverType = driverInterface::OBOS_SERVICE_TYPE_USER_INPUT_DEVICE,
	.requests = 0,
	.functionTable = {
		.GetServiceType = []()->driverInterface::serviceType { return driverInterface::serviceType::OBOS_SERVICE_TYPE_USER_INPUT_DEVICE; },
		.serviceSpecific = {
			.userInputDevice = {
				.ReadCharacter = nullptr,
				.unused = {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,}
			},
		},
	},
};
#pragma GCC diagnostic pop

extern "C" void _start()
{
	new (&g_keyBuffer) utils::Vector<uint16_t>{};

	// Register the IRQ
	RegisterInterruptHandler(0x21, keyboardInterrupt);
	MapIRQToVector(1, 0x21);

	// Keys need to held for 250 ms before repeating, and they repeat at a rate of 30 hz (33.33333 ms).
	sendCommand(2, 0xF3, 0);

	// Set scancode set 1.
	sendCommand(2, 0xF0, 0);

	// Enable scanning.
	sendCommand(1, 0xF4);

	// Clear all keyboard LEDs.
	sendCommand(2, 0xED, 0b000);

	g_driverHeader.driver_initialized = true;
	while (!g_driverHeader.driver_finished_loading);
	thread::ExitThread(0);
}

static bool isExtendedScancode = false;
static void keyboardInterrupt(interrupt_frame*)
{
	byte scancode = inb(0x60);

	char ch = 0;
	
	if (scancode == 0xE0)
		isExtendedScancode = true;
	if (scancode > 0xD8)
	{
		SendEOI();
		return;
	}
	if (isExtendedScancode)
	{
		bool wasReleased = (scancode & 0x80) == 0x80;
		if (wasReleased)
			scancode &= ~(0x80);
		if (g_keys[scancode].skipExtended)
		{
			isExtendedScancode = false;
			goto skip_extended_processing;
		}
		driverInterface::SpecialKeys key = driverInterface::SpecialKeys::INVALID;
		if (scancode == 0x5b)
			key = driverInterface::SpecialKeys::LEFT_GUI;
		else if (scancode == 0x5c)
			key = driverInterface::SpecialKeys::RIGHT_GUI;
		else
			key = g_keys[scancode].extendedCh;
		g_keyBuffer.push_back((uint16_t)key);
		isExtendedScancode = false;
		SendEOI();
		return;
	}

	skip_extended_processing:
	bool wasReleased = (scancode & 0x80) == 0x80;
	if (wasReleased)
		scancode &= ~(0x80);

	ch = g_keys[scancode].ch;

	g_keys[scancode].isPressed = !wasReleased;
	g_keys[scancode].nPressed++;
	if (g_keys[scancode].isPressed)
	{
		if (scancode == 0x1d)
		{
			g_keyBuffer.push_back(ch << 8);
			goto skip;
		}
		if (ch)
		{
			bool isUppercase = flags.isCapsLock || flags.isShiftPressed;
			if (flags.isCapsLock && flags.isShiftPressed)
				isUppercase = false;

			char newKey = 0;
			if (ch < 'A' || ch > 'Z')
				newKey = ch;
			else
				newKey = isUppercase ? ch : (ch - 'A') + 'a';
			if (flags.isShiftPressed && (ch < 'A' || ch > 'Z') && g_keys[scancode].shiftAlias)
				newKey = g_keys[scancode].shiftAlias;
			if (newKey)
			{
				g_keyBuffer.push_back(newKey);
				g_kernelConsole.ConsoleOutput(newKey);
			}
		}
		else if (g_keys[scancode].extendedCh != driverInterface::SpecialKeys::INVALID)
			g_keyBuffer.push_back((uint16_t)g_keys[scancode].extendedCh);
	}

	skip:
	if (scancode == 0x2A || scancode == 0x36)
		flags.isShiftPressed = !wasReleased;

	if (wasReleased)
	{
		g_keys[scancode].nPressed = 0;
		SendEOI();
		return;
	}

	switch (scancode)
	{
	case 0x3A:
	{
		flags.isCapsLock = !flags.isCapsLock;
		byte bitfield = flags.isScrollLock | (flags.isNumberLock << 1) | (flags.isCapsLock << 2);
		sendCommand(2, 0xED, bitfield);
		break;
	}
	case 0x46:
	{
		flags.isScrollLock = !flags.isScrollLock;
		byte bitfield = flags.isScrollLock | (flags.isNumberLock << 1) | (flags.isCapsLock << 2);
		sendCommand(2, 0xED, bitfield);
		break;
	}
	case 0x45:
	{
		flags.isNumberLock = !flags.isNumberLock;
		byte bitfield = flags.isScrollLock | (flags.isNumberLock << 1) | (flags.isCapsLock << 2);
		sendCommand(2, 0xED, bitfield);
		break;
	}
	default:
		break;
	}
	SendEOI();
}

byte sendCommand(uint32_t nCommands, ...)
{
	byte ret = 0x00;
	va_list list;
	utils::Vector<byte> commands;
	va_start(list, nCommands);
	for (uint32_t x = 0; x < nCommands; x++)
		commands.push_back(va_arg(list, int));
	va_end(list);
	for (int i = 0; i < 5; i++)
	{
		for (auto& c : commands)
		{
			while ((inb(0x64) & 0b10) == 0b10);
			outb(0x60, c);
		}
		ret = inb(0x60);
		if (ret == RESEND && commands[0] != 0xF0 /* Set scancode set.*/)
			for (size_t i = 0; ret != ACK && i < 10; ret = inb(0x60), i++);
		if (ret == ACK)
		{
			va_end(list);
			break;
		}

	}
	return ret;
}