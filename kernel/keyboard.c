#include "keyboard.h"

#include "kalloc.h"
#include "interrupts.h"
#include "inline-asm.h"
#include "klog.h"
#include "liballoc/liballoc_1_1.h"

static BYTE* keyboardBuffer = NULLPTR;
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

void keyboardIrqHandler(int interrupt, isr_registers registers)
{
	disablePICInterrupt(1);
	
	//BYTE scanCode = inb(0x60);

	enablePICInterrupt(1);
}

INT kKeyboardInit(SIZE_T bufferSize)
{
	if (keyboardFlags.initialized)
		return -1;
	disablePICInterrupt(1);
	keyboardBuffer = kcalloc(bufferSize, sizeof(char));
	int interrupt = 1;
	setPICInterruptHandlers(&interrupt, 1, keyboardIrqHandler);

	// Use the second scan code table.
	while (!(inb(0x64) & 0b10));
	outb(0x60, 0xF0);
	outb(0x60, 0x02);
	while (!(inb(0x64) & 0b1));
	kassert(inb(0x64) == 0xFA, KSTR_LITERAL("ACK was not received from the keyboard.\r\n"));
	// Clear the light emitting diodes' (led) state
	while (!(inb(0x64) & 0b10));
	outb(0x60, 0xED);
	outb(0x60, 0xFF);
	while (!(inb(0x64) & 0b1));
	kassert(inb(0x64) == 0xFA, KSTR_LITERAL("ACK was not received from the keyboard.\r\n"));
	// Make keys repeat with an interval of 30 hz (33.33 ms), waiting 250 ms before keys start repeating.
	while (!(inb(0x64) & 0b10));
	outb(0x60, 0xED);
	outb(0x60, 0x00);
	while (!(inb(0x64) & 0b1));
	kassert(inb(0x64) == 0xFA, KSTR_LITERAL("ACK was not received from the keyboard.\r\n"));
	// Enable scanning
	while (!(inb(0x64) & 0b10));
	outb(0x60, 0xF4);
	io_wait();
	io_wait();
	kassert(inb(0x64) == 0xFA, KSTR_LITERAL("ACK was not received from the keyboard.\r\n"));
	
	enablePICInterrupt(1);
	keyboardFlags.initialized = TRUE;
	return 0;
}