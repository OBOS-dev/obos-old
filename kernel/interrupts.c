/*
	interrupts.c

	Copyright (c) 2023 Omar Berrow
*/

#include "interrupts.h"

#include "types.h"
#include "inline-asm.h"
#include "klog.h"

#define PIC1		0x20
#define PIC2		0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1+1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2+1)

#define ICW1_ICW4	0x01
#define ICW1_SINGLE	0x02
#define ICW1_INTERVAL4	0x04
#define ICW1_LEVEL	0x08
#define ICW1_INIT	0x10

#define ICW4_8086	0x01
#define ICW4_AUTO	0x02
#define ICW4_BUF_SLAVE	0x08
#define ICW4_BUF_MASTER	0x0C
#define ICW4_SFNM	0x10

static void(*picCallbacks[16])(int interrupt, isr_registers registers);
static void(*exceptionHandlers[30])(int interrupt, int ec, isr_registers registers);

extern void idtUpdate();

void defaultPicInterruptCallback(int interrupt, isr_registers registers)
{
	if(interrupt != 0)
		klog_warning("Unhandled interrupt from the PIC. Interrupt code: 0x%X\r\n", interrupt);
	// The keyboard interrupt needs to be cleared if it was a interrupt from the keyboard.
	if (interrupt == 1)
		(void)inb(0x60);
}

void defaultExceptionHandler(int interrupt, int ec, isr_registers registers)
{
	klog_error("Unhandled exception 0x%X. Error code: %d. Dumping registers...\r\n"
			   "\tEDI: 0x%X\r\n\tESI: 0x%X\r\n\tEBP: 0x%X\r\n\tESP: 0x%X\r\n\tEBX: 0x%X\r\n\tEDX: 0x%X\r\n\tECX: 0x%X\r\n\tEAX: 0x%X\r\n"
			   "\tEIP: 0x%X\r\n\tCS: 0x%X\r\n\tEFLAGS: 0x%X\r\n\tSS: 0x%X\r\n"
			   "\tDS: 0x%X\r\n",
		interrupt, ec,
		registers.edi, registers.esi, registers.ebp, registers.esp, registers.ebx, registers.edx, registers.ecx, registers.eax,
		registers.eip, registers.cs, registers.eflags, registers.ss,
		registers.ds);
	printStackTrace();
	kpanic(NULLPTR, 0);
}

void defaultInterruptHandler(isr_registers registers)
{
	if(registers.intNumber > 31 && registers.intNumber < 48)
	{
		picCallbacks[registers.intNumber - 32](registers.intNumber - 32, registers);
		outb(0x20, 0x20);
		if(registers.intNumber > 40)
			outb(0xA0, 0x20);
	}
	if (registers.intNumber < 31)
	{
		int index = registers.intNumber;
		exceptionHandlers[index](registers.intNumber, registers.errorCode, registers);
	}
	if (registers.intNumber == 48)
		picCallbacks[0](48, registers); // Call the scheduler.
}

void initPIC()
{
	// Initialize in cascade mode.
	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	io_wait();

	// Set the three initialization words.
	outb(PIC1_DATA, 0x20);
	io_wait();
	outb(PIC2_DATA, 0x28);
	io_wait();
	outb(PIC1_DATA, 0b00000100);
	io_wait();
	outb(PIC2_DATA, 0b00000010);
	io_wait();
	outb(PIC1_DATA, ICW4_8086);
	io_wait();
	outb(PIC2_DATA, ICW4_8086);
	io_wait();

	// Disable all PIC interrupts.
	outb(PIC1_DATA, 0xFF);
	outb(PIC2_DATA, 0xFF);
}

typedef struct __idtEntry
{
	UINT16_T base_lowBytes;
	UINT16_T sel;
	UINT8_T  always0;
	UINT8_T  flags;
	UINT16_T base_highBytes;
} __attribute__((packed)) idtEntry;

static idtEntry idtEntries[256];

struct __idt_ptr
{
	UINT16_T limit;
	UINT32_T base;
} __attribute__((packed));

_Static_assert(sizeof(struct __idt_ptr) == 6, "sizeof(struct __idt_ptr) is not six!");

static struct __idt_ptr idt_ptr;

static void _setIdtEntry(UINT8_T index, UINT32_T base, UINT16_T sel, UINT8_T flags)
{
	idtEntries[index].base_lowBytes = base & 0xFFFF;
	idtEntries[index].base_highBytes = (base >> 16) & 0xFFFF;

	idtEntries[index].sel = sel;
	idtEntries[index].always0 = 0;
	idtEntries[index].flags = flags | 0x60;
}

#define _setInterruptHandler(interrupt) extern void isr ##interrupt(); \
										_setIdtEntry(interrupt, (UINT32_T)isr##interrupt, 0x08, 0x8E)

void initIDT()
{
	idt_ptr.limit = sizeof(idtEntries) - 1;
	idt_ptr.base = (UINT32_T)&idtEntries;

	{
		char* tmp = (PCHAR)&idtEntries[0];
		for (int i = 0; i < sizeof(idtEntries); tmp[i++] = 0);
	}

#pragma region Set the IDT entries.
	_setInterruptHandler(0);
	_setInterruptHandler(1);
	_setInterruptHandler(2);
	_setInterruptHandler(3);
	_setInterruptHandler(4);
	_setInterruptHandler(5);
	_setInterruptHandler(6);
	_setInterruptHandler(7);
	_setInterruptHandler(8);
	_setInterruptHandler(9);
	_setInterruptHandler(10);
	_setInterruptHandler(11);
	_setInterruptHandler(12);
	_setInterruptHandler(13);
	_setInterruptHandler(14);
	_setInterruptHandler(15);
	_setInterruptHandler(16);
	_setInterruptHandler(17);
	_setInterruptHandler(18);
	_setInterruptHandler(19);
	_setInterruptHandler(20);
	_setInterruptHandler(21);
	_setInterruptHandler(22);
	_setInterruptHandler(23);
	_setInterruptHandler(24);
	_setInterruptHandler(25);
	_setInterruptHandler(26);
	_setInterruptHandler(27);
	_setInterruptHandler(28);
	_setInterruptHandler(30);
	_setInterruptHandler(31);
	_setInterruptHandler(32);
	_setInterruptHandler(33);
	_setInterruptHandler(34);
	_setInterruptHandler(35);
	_setInterruptHandler(36);
	_setInterruptHandler(37);
	_setInterruptHandler(38);
	_setInterruptHandler(39);
	_setInterruptHandler(40);
	_setInterruptHandler(41);
	_setInterruptHandler(42);
	_setInterruptHandler(43);
	_setInterruptHandler(44);
	_setInterruptHandler(45);
	_setInterruptHandler(46);
	_setInterruptHandler(47);
	_setInterruptHandler(48);
	_setInterruptHandler(64);
	_setInterruptHandler(80);
#pragma endregion

	idtUpdate((UINT32_T)&idt_ptr);
}

void setIdtEntry(UINT8_T index, UINT32_T base, UINT16_T sel, UINT8_T flags)
{
	_setIdtEntry(index, base, sel, flags);
	idtUpdate((UINT32_T)&idt_ptr);
}

void initInterrupts()
{
	initIDT();

	initPIC();

	for (int i = 0; i < 16; i++)
		picCallbacks[i] = defaultPicInterruptCallback;
	for (int i = 0; i < 30; i++)
		exceptionHandlers[i] = defaultExceptionHandler;
	// Finally, turn on interrupts.
	asm volatile("sti");
}

void enablePICInterrupt(UINT8_T IRQline)
{
	UINT16_T port = PIC1_DATA;
	UINT8_T value = 0;

	if (IRQline > 8)
	{
		port = PIC2_DATA;
		IRQline -= 8;
	}
	value = inb(port) & ~(1 << IRQline);
	outb(port, value);
}
void disablePICInterrupt(UINT8_T IRQline)
{
	UINT16_T port = PIC1_DATA;
	UINT8_T value = 0;

	if (IRQline > 8)
	{
		port = PIC2_DATA;
		IRQline -= 8;
	}

	value = inb(port) | (1 << IRQline);
	outb(port, value);
}
void setPICInterruptHandlers(int* interrupts, int nInterrupts, void(*isr)(int interrupt, isr_registers registers))
{
	for (int i = 0; i < nInterrupts; i++)
		if (interrupts[i] < sizeof(picCallbacks) / sizeof(void(*)(int)))
			picCallbacks[interrupts[i]] = isr;
}
void resetPICInterruptHandlers(int* interrupts, int nInterrupts)
{
	for (int i = 0; i < nInterrupts; i++)
		if (interrupts[i] < sizeof(picCallbacks) / sizeof(void(*)(int)))
			picCallbacks[interrupts[i]] = defaultPicInterruptCallback;
}
void setExceptionHandlers(int* interrupts, int nInterrupts, void(*handler)(int interrupt, int ec, isr_registers registers))
{
	for (int i = 0; i < nInterrupts; i++)
		if (interrupts[i] < (sizeof(picCallbacks) / sizeof(void(*)(int, int, isr_registers))))
			exceptionHandlers[interrupts[i]] = handler;
}

void resetExceptionHandlers(int* interrupts, int nInterrupts)
{
	for (int i = 0; i < nInterrupts; i++)
		if (interrupts[i] < sizeof(picCallbacks) / sizeof(void(*)(int, int, isr_registers)))
			exceptionHandlers[interrupts[i]] = defaultExceptionHandler;
}
