#include <stdbool.h>

#include "terminal.h"
#include "types.h"
#include "klog.h"
#include "kalloc.h"
#include "inline-asm.h"
#include "kserial.h"
#include "gdt.h"
#include "interrupts.h"

#include "multiboot.h"

#if defined(__linux__)
#error Must be compiled with a cross compiler.
#endif
#if !defined(__i686__)
#error Must be compiled with a i686 compiler.
#endif
#if defined(__STDC_NO_VLA__)
#error VLA must be supported on the compiler
#endif

_Static_assert (sizeof(INT8_T) == 1, "INT8_T needs to be one byte.");
_Static_assert (sizeof(INT16_T) == 2, "INT16_T needs to be two bytes.");
_Static_assert (sizeof(INT32_T) == 4, "INT32_T needs to be four bytes.");
_Static_assert (sizeof(INT64_T) == 8, "INT64_T needs to be eight bytes.");

extern int acpiEnable(void);
extern int initAcpi(void);
extern int acpiEnable(void);
extern void acpiPowerOff(void);

multiboot_info_t* g_multibootInfo = (multiboot_info_t*)0;

static inline char* strcpy(char* destination, const char* source, int bytesToCopy)
{
	for(int i = 0; i < bytesToCopy; i++)
		destination[i] = source[i];
	return destination;
}

// int pow(int a, int b)
// {
// 	int res = a;
// 	for (int i = 0; i < b; i++) res *= a;
// 	res /= a;
// 	return res;
// }

static BOOL shutdownTimerInitialized = FALSE;
static SIZE_T irqIterations = 0;

static void shutdownComputerInterrupt(int interrupt, isr_registers registers)
{
	if (!shutdownTimerInitialized)
		return;
	shutdownTimerInitialized = irqIterations++ != 250;
	if (!shutdownTimerInitialized)
	{
		int irq0 = 0;
		resetPICInterruptHandlers(&irq0, 1);

		acpiPowerOff();
	}
}
static void onKernelPanic()
{
	TerminalSetColor(TERMINALCOLOR_COLOR_WHITE | TERMINALCOLOR_COLOR_BLACK >> 4);
	TerminalOutputString("\r\n");
	outb(COM1, '\r');
	outb(COM1, '\n');
	outb(COM2, '\r');
	outb(COM2, '\n');
	klog_info("Shuting down the computer in five seconds.\r\n");
	{
		int _irq0 = 0;
		setPICInterruptHandlers(&_irq0, 1, shutdownComputerInterrupt);

		UINT32_T divisor = 1193180 / 50;

		outb(0x43, 0x36);

		UINT8_T l = (UINT8_T)(divisor & 0xFF);
		UINT8_T h = (UINT8_T)((divisor >> 8) & 0xFF);

		// Send the frequency divisor.
		outb(0x40, l);
		outb(0x40, h);
		shutdownTimerInitialized = TRUE;
	}
	while (shutdownTimerInitialized)
		asm volatile ("hlt");
}

volatile BOOL interruptsWork = FALSE;

static void testHandler(int interrupt, int ec, isr_registers registers)
{
	klog_info("Software interrupts work.\r\n");
	interruptsWork = TRUE;
}

static void int3Handler(int interrupt, int ec, isr_registers registers)
{
	klog_info("Breakpoint interrupt at %p.\r\n", registers.eip, registers.ebp);
	klog_info("Dumping registers...\r\n"
		"\tEDI: 0x%X\r\n\tESI: 0x%X\r\n\tEBP: 0x%X\r\n\tESP: 0x%X\r\n\tEBX: 0x%X\r\n\tEDX: 0x%X\r\n\tECX: 0x%X\r\n\tEAX: 0x%X\r\n"
		"\tEIP: 0x%X\r\n\tCS: 0x%X\r\n\tEFLAGS: 0x%X\r\n\tUSERESP: 0x%X\r\n\tSS: 0x%X\r\n"
		"\tDS: 0x%X\r\n",
		registers.edi, registers.esi, registers.ebp, registers.esp, registers.ebx, registers.edx, registers.ecx, registers.eax,
		registers.eip, registers.cs, registers.eflags, registers.useresp, registers.ss,
		registers.ds);
	printStackTrace();
	nop();
}

static void irq1Handler(int interrupt, isr_registers registers)
{
	(void)inb(0x60);
}

void kmain(multiboot_info_t* mbd, UINT32_T magic)
{
	g_multibootInfo = mbd;

	initGdt();
	initInterrupts();

	InitializeTeriminal(TERMINALCOLOR_COLOR_WHITE | TERMINALCOLOR_COLOR_BLACK << 4);
	initAcpi();
	acpiEnable();
	
	setOnKernelPanic(onKernelPanic);

	kpanic(KSTR_LITERAL("Testing panic."));

	if (mbd->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
		klog_info("The bootloader's name is, \"%s.\"\r\n", mbd->boot_loader_name);

	{
		int interrupt = 3;
		setExceptionHandlers(&interrupt, 1, testHandler);
		asm volatile("int3");
		kassert(interruptsWork, KSTR_LITERAL("Software interrupts do not work on this computer!\r\n"));
		setExceptionHandlers(&interrupt, 1, int3Handler);
		interrupt = 1;
		setPICInterruptHandlers(&interrupt, 1, irq1Handler);
	}
	
	kassert(magic == MULTIBOOT_BOOTLOADER_MAGIC, KSTR_LITERAL("Invalid magic number for multiboot.\r\n"));
	kassert(mbd->flags & MULTIBOOT_INFO_MEM_MAP, KSTR_LITERAL("No memory map provided from the bootloader.\r\n"));
	kassert(mbd->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO, KSTR_LITERAL("No framebuffer info provided from the bootloader.\r\n"));
	kassert(mbd->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT, KSTR_LITERAL("The video type isn't MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT\r\n"));

	kinitserialports();

	if (InitSerialPort(COM1, MAKE_BAUDRATE_DIVISOR(115200), SEVEN_DATABITS, ONE_STOPBIT, PARITYBIT_EVEN, 1024, 1024) != 0)
		klog_warning("Could not initalize COM1.\r\n");
	if (InitSerialPort(COM2, MAKE_BAUDRATE_DIVISOR(115200), SEVEN_DATABITS, ONE_STOPBIT, PARITYBIT_EVEN, 1024, 1024) != 0)
		klog_warning("Could not initalize COM2.\r\n");

	kmeminit();
	{
		
	}

	while(IsSerialPortInitialized(COM2) == TRUE)
		if(inb(0x2F8 + 5) & 0x1)
			TerminalOutputCharacter(inb(0x2F8));

	while (1);

	acpiPowerOff();

	klog_warning("Control flow reaches end of kmain.");
}