#include <stdbool.h>

#include "terminal.h"
#include "types.h"
#include "klog.h"
#include "kalloc.h"
#include "inline-asm.h"
#include "kserial.h"

#include "multiboot/mutliboot.h"

#if defined(__linux__)
#error Must be compiled with a cross compiler.
#endif
#if !defined(__i686__)
#error Must be compiled with a i686 compiler.
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

static void onKernelPanic()
{
	TerminalSetColor(TERMINALCOLOR_COLOR_WHITE | TERMINALCOLOR_COLOR_BLACK >> 4);
	klog_info("\r\nShuting down the computer.\r\n");
	for(int i = 0; i < 0x20000000; i++)
		nop();
	acpiPowerOff();
}

static void defaultInterruptHandler()
{
	nop();
}

void kmain(multiboot_info_t* mbd, UINT32_T magic)
{
	g_multibootInfo = mbd;

	InitializeTeriminal(TERMINALCOLOR_COLOR_WHITE | TERMINALCOLOR_COLOR_BLACK << 4);
	initAcpi();
	acpiEnable();
	
	setOnKernelPanic(onKernelPanic);

	kassert(magic == MULTIBOOT_BOOTLOADER_MAGIC, "Invalid magic number for multiboot.\r\n", 37);
	kassert(mbd->flags >> 6 & 0x1, "No memory map provided from GRUB.\r\n", 35);
	kassert(mbd->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT, KSTR_LITERAL("The video type isn't MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT\r\n"));


	{
		void(**interrupt_vector_table)() = 0x00000000;
		for(int i = 0; i <= 0x3FF / 4; interrupt_vector_table[++i] = defaultInterruptHandler);
		__asm__("sti");
	}

	kinitserialports();

	kmeminit();
	{
		SIZE_T heapSize = 0;
		// Find the bigest block of memory.
		klog_info("Finding the biggest memory block.\r\n");
		PVOID heapStorageBlock = kfindmemblock(-1, &heapSize);
		kassert(heapSize != 0, KSTR_LITERAL("The heap's size is zero."));
		heapSize = ((INT)(heapSize * 0.001953125)) * 512;
		klog_info("Initalizing the heap.\r\n");
		kheapinit(heapStorageBlock, heapSize);
	}

	klog_info("Allocating a memory block on heap.\r\n");
	STRING test = kheapalloc(sizeof("Hello, world!\r\n"), 0);
	kassert(test != (PVOID)0xFFFFFFFF, KSTR_LITERAL("test != 0xFFFFFFFF\r\n"));
	strcpy(test, KSTR_LITERAL("Hello, world!\r\n"));
	TerminalOutputString(test);

	while(1)
		if(inb(0x2F8 + 5) & 0x1)
			TerminalOutputCharacter(inb(0x2F8));

	acpiPowerOff();
}