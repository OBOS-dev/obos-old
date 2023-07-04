#include "keyboard.h"

#include "interrupts.h"
#include "inline-asm.h"
#include "klog.h"
#include "paging.h"
#include "liballoc/liballoc_1_1.h"

static volatile BYTE* s_keyboardBuffer = NULLPTR;
static volatile BYTE* s_keyboardBufferLocation = NULLPTR;

struct
{
	BOOL initialized : 1;
	BOOL scrollLock : 1;
	BOOL numberLock : 1;
	BOOL capsLock : 1;
	BOOL shiftPressed : 1;
} keyboardFlags = { 0,0,0,0,0 };

typedef struct __scanCode
{
	// Maximum size for a scan code is eight bytes.
	BYTE bytes[8];
} __attribute__((packed)) scanCode_t;

//static scanCode_t g_scanCodesSet2[] = {
//
//};

static void sendByteToKeyboard(BYTE val)
{
	while (!(inb(0x64) & 0b10));
	outb(0x60, val);
}

void keyboardIrqHandler(int interrupt, isr_registers registers)
{
	disablePICInterrupt(1);
	
	//BYTE scanCode = inb(0x60);

	enablePICInterrupt(1);
}

INT kKeyboardInit()
{
	if (keyboardFlags.initialized)
		return -1;
	disablePICInterrupt(1);
	s_keyboardBufferLocation = s_keyboardBuffer = kalloc_pages(2);
	int interrupt = 1;
	setPICInterruptHandlers(&interrupt, 1, keyboardIrqHandler);

	// Use the second scan code table.
	sendByteToKeyboard(0xF0);
	sendByteToKeyboard(0x02);
	while (!(inb(0x64) & 0b1));
	kassert(inb(0x64) == 0xFA, KSTR_LITERAL("ACK was not received from the keyboard.\r\n"));
	// Clear the light emitting diodes' (led) state.
	sendByteToKeyboard(0xED);
	sendByteToKeyboard(0xFF);
	while (!(inb(0x64) & 0b1));
	kassert(inb(0x64) == 0xFA, KSTR_LITERAL("ACK was not received from the keyboard.\r\n"));
	// Make keys repeat with an interval of 30 hz (33.33 ms), waiting 250 ms before keys start repeating.
	sendByteToKeyboard(0xED);
	sendByteToKeyboard(0x00);
	while (!(inb(0x64) & 0b1));
	kassert(inb(0x64) == 0xFA, KSTR_LITERAL("ACK was not received from the keyboard.\r\n"));
	// Enable scanning
	sendByteToKeyboard(0xF4);
	while (!(inb(0x64) & 0b1));
	kassert(inb(0x64) == 0xFA, KSTR_LITERAL("ACK was not received from the keyboard.\r\n"));
	keyboardFlags.initialized = TRUE;
	
	enablePICInterrupt(1);
	return 0;
}