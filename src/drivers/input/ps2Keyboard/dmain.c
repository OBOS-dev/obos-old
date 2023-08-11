/*
	dmain.cpp
	
	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>

#include <driver_api/enums.h>

#include <stdarg.h>

typedef _Bool bool;

extern enum exitStatus RegisterDriver(DWORD driverID, enum serviceType type);
extern enum exitStatus RegisterInterruptHandler(DWORD driverId, BYTE interruptId, void(*handler)(const struct interrupt_frame* frame));
extern enum exitStatus PicSendEoi(BYTE irq);
extern enum exitStatus DisableIRQ(BYTE irq);
extern enum exitStatus EnableIRQ(BYTE irq);
extern enum exitStatus RegisterReadCallback(DWORD driverId, void(*callback)(STRING outputBuffer, SIZE_T sizeBuffer));
extern enum exitStatus PrintChar(CHAR ch, bool flush);

extern enum exitStatus CallSyscall(DWORD syscallId, ...);

#define cli() asm volatile("cli")
#define sti() asm volatile("sti")

#define DRIVER_ID 0
#define true 1
#define false 0
#define ACK 0xfa

void keyboardInterrupt(const struct interrupt_frame* frame);

BYTE sendCommand(DWORD nCommands, ...);

void outb(UINT16_T port, UINT8_T val);
void outw(UINT16_T port, UINT16_T val);
UINT8_T inb(UINT16_T port);
UINT16_T inw(UINT16_T port);

typedef struct _key
{
	DWORD scanCode;
	SIZE_T nPressed;
	bool isPressed;
	char ch;
} key;

key g_keys[] = {
	{ 0x00, 0,0, '\x00' },
	{ 0x01, 0,0, '\x1b' },
	{ 0x02, 0,0, '1' },
	{ 0x03, 0,0, '2' },
	{ 0x04, 0,0, '3' },
	{ 0x05, 0,0, '4' },
	{ 0x06, 0,0, '5' },
	{ 0x07, 0,0, '6' },
	{ 0x08, 0,0, '7' },
	{ 0x09, 0,0, '8' },
	{ 0x0A, 0,0, '9' },
	{ 0x0B, 0,0, '0' },
	{ 0x0C, 0,0, '-' },
	{ 0x0D, 0,0, '=' },
	{ 0x0E, 0,0, '\b' },
	{ 0x0F, 0,0, '\t' },
	{ 0x10, 0,0, 'Q' },
	{ 0x11, 0,0, 'W' },
	{ 0x12, 0,0, 'E' },
	{ 0x13, 0,0, 'R' },
	{ 0x14, 0,0, 'T' },
	{ 0x15, 0,0, 'Y' },
	{ 0x16, 0,0, 'U' },
	{ 0x17, 0,0, 'I' },
	{ 0x18, 0,0, 'O' },
	{ 0x19, 0,0, 'P' },
	{ 0x1A, 0,0, '[' },
	{ 0x1B, 0,0, ']' },
	{ 0x1C, 0,0, '\n' },
	{ 0x1D, 0,0, '\0' },
	{ 0x1E, 0,0, 'A' },
	{ 0x1F, 0,0, 'S' },
	{ 0x20, 0,0, 'D' },
	{ 0x21, 0,0, 'F' },
	{ 0x22, 0,0, 'G' },
	{ 0x23, 0,0, 'H' },
	{ 0x24, 0,0, 'J' },
	{ 0x25, 0,0, 'K' },
	{ 0x26, 0,0, 'L' },
	{ 0x27, 0,0, ';' },
	{ 0x28, 0,0, '\'' },
	{ 0x29, 0,0, '`' },
	{ 0x2A, 0,0, '\0' },
	{ 0x2B, 0,0, '\\' },
	{ 0x2C, 0,0, 'Z' },
	{ 0x2D, 0,0, 'X' },
	{ 0x2E, 0,0, 'C' },
	{ 0x2F, 0,0, 'V' },
	{ 0x30, 0,0, 'B' },
	{ 0x31, 0,0, 'N' },
	{ 0x32, 0,0, 'M' },
	{ 0x33, 0,0, ',' },
	{ 0x34, 0,0, '.' },
	{ 0x35, 0,0, '/' },
	{ 0x36, 0,0, '\0' },
	{ 0x37, 0,0, '*' },
	{ 0x38, 0,0, '\0' },
	{ 0x39, 0,0, ' ' },
	{ 0x3A, 0,0, '\0' },
	{ 0x3B, 0,0, '\0' },
	{ 0x3C, 0,0, '\0' },
	{ 0x3D, 0,0, '\0' },
	{ 0x3E, 0,0, '\0' },
	{ 0x3F, 0,0, '\0' },
	{ 0x40, 0,0, '\0' },
	{ 0x41, 0,0, '\0' },
	{ 0x42, 0,0, '\0' },
	{ 0x43, 0,0, '\0' },
	{ 0x44, 0,0, '\0' },
	{ 0x45, 0,0, '\0' },
	{ 0x46, 0,0, '\0' },
	{ 0x47, 0,0, '7' },
	{ 0x48, 0,0, '8' },
	{ 0x49, 0,0, '9' },
	{ 0x4A, 0,0, '-' },
	{ 0x4B, 0,0, '4' },
	{ 0x4C, 0,0, '5' },
	{ 0x4D, 0,0, '6' },
	{ 0x4E, 0,0, '+' },
	{ 0x4F, 0,0, '1' },
	{ 0x50, 0,0, '2' },
	{ 0x51, 0,0, '3' },
	{ 0x52, 0,0, '0' },
	{ 0x53, 0,0, '.' },
	{ 0x54, 0,0, '\0' },
	{ 0x55, 0,0, '\0' },
	{ 0x56, 0,0, '\0' },
	{ 0x57, 0,0, '\0' },
	{ 0x58, 0,0, '\0' },
};
// Should take 2 pages.
key g_keyBuffer[512] = {};
SIZE_T g_keyBufferPosition = 0;

struct
{
	bool isScrollLock : 1;
	bool isNumberLock : 1;
	bool isCapsLock : 1;
	bool isShiftPressed : 1;
} __attribute__((packed)) flags = {0,0,0};


static void readCallback(STRING outputBuffer, SIZE_T size)
{
	for (SIZE_T i = 0; i < size; i++)
		outputBuffer[i] = g_keyBuffer[i].ch;
}

extern void _init();

int _start()
{
	_init();

	RegisterDriver(DRIVER_ID, SERVICE_TYPE_USER_INPUT_DEVICE);

	cli();
	DisableIRQ(1);

	// Keys need to helf for 250 ms before repeating, and they repeat at a rate 30 hz (33.33333 ms).
	sendCommand(2, 0xF3, 0);
	
	// Turn off the leds.
	sendCommand(2, 0xED, 0);

	// Set scancode set 1.
	sendCommand(2, 0xF0, 2);

	// Enable scanning.
	//sendCommand(1, 0xF4);

	RegisterInterruptHandler(DRIVER_ID, 0x21, keyboardInterrupt);

	RegisterReadCallback(DRIVER_ID, readCallback);

	EnableIRQ(1);
	sti();

	return 0;
}

void keyboardInterrupt(const struct interrupt_frame* frame)
{
	DisableIRQ(1);

	BYTE scancode = inb(0x60);

	bool wasReleased = (scancode & 0x80) == 0x80;

	const char ch = g_keys[scancode].ch;
	
	if (scancode > 0xD8)
		goto done;

	if (wasReleased)
		scancode &= ~(0x80);
	g_keys[scancode].isPressed = !wasReleased;
	g_keys[scancode].nPressed++;
	if(g_keys[scancode].ch)
	{
		bool isUppercase = flags.isCapsLock || flags.isShiftPressed;
		if (flags.isCapsLock && flags.isShiftPressed)
			isUppercase = false;

		key newKey = {};
		if(ch < 'A' || ch > 'Z')
			newKey.ch = ch;
		else
			newKey.ch = isUppercase ? ch : (ch - 'A') + 'a';
		g_keyBuffer[g_keyBufferPosition++] = newKey;
		if(newKey.ch != '\n')
			PrintChar(newKey.ch, false);
		else
		{
			PrintChar('\r', false);
			PrintChar('\n', true);
		}
	}

	if (scancode == 0x2A || scancode == 0x36)
		flags.isShiftPressed = !wasReleased;

	if (wasReleased)
		goto done;


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

	done:
	EnableIRQ(1);
	PicSendEoi(1);
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

void outb(UINT16_T port, UINT8_T val)
{
	asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}
void outw(UINT16_T port, UINT16_T val)
{
	asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port) : "memory");
}
UINT8_T inb(UINT16_T port)
{
	volatile UINT8_T ret;
	asm volatile ("inb %1, %0"
		: "=a"(ret)
		: "Nd"(port)
		: "memory");
	return ret;
}
UINT16_T inw(UINT16_T port)
{
	volatile UINT16_T ret;
	asm volatile ("inw %1, %0"
		: "=a"(ret)
		: "Nd"(port)
		: "memory");
	return ret;
}

__asm__("RegisterDriver: ;"
			 "mov $0, %eax;"
			 "int $0x50;"
			 "ret;"
		"RegisterInterruptHandler: ;"
			 "mov $1, %eax;"
			 "int $0x50;"
			 "ret;"
		"PicSendEoi:"
			"mov $2, %eax;"
			"int $0x50;"
			"ret;"
		"EnableIRQ:"
			"mov $3, %eax;"
			"int $0x50;"
			"ret;"
		"DisableIRQ:"
			"mov $4, %eax;"
			"int $0x50;"
			"ret;"
		"RegisterReadCallback:"
			"mov $5, %eax;"
			"int $0x50;"
			"ret;"
		"PrintChar:"
			"mov $6, %eax;"
			"int $0x50;"
			"ret;"
		"CallSyscall: ;"
			 "mov 0x8(%esp),%eax;"
			 "int $0x50;"
			 "ret;");