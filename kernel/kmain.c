/*
    kmain.c

    Copyright (c) 2023 Omar Berrow
*/

#include <stdbool.h>

#include "terminal.h"
#include "types.h"
#include "klog.h"
#include "paging.h"
#include "inline-asm.h"
#include "kserial.h"
#include "gdt.h"
#include "interrupts.h"
#include "bitfields.h"
#include "multitasking.h"
#include "initrd.h"

#include "multiboot.h"

#include "liballoc/liballoc_1_1.h"

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
_Static_assert (sizeof(BOOL) == 1, "BOOL needs to be one byte.");

extern int acpiEnable(void);
extern int initAcpi(void);
extern int acpiEnable(void);
extern void acpiPowerOff(void);

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
    klog_info("Shutting down the computer in five seconds.\r\n");
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
        sti();
        enablePICInterrupt(0);
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
    klog_info("Breakpoint interrupt at %p.\r\n", (PVOID)registers.eip);
    klog_info("Dumping registers...\r\n"
        "\tEDI: 0x%X\r\n\tESI: 0x%X\r\n\tEBP: 0x%X\r\n\tESP: 0x%X\r\n\tEBX: 0x%X\r\n\tEDX: 0x%X\r\n\tECX: 0x%X\r\n\tEAX: 0x%X\r\n"
        "\tEIP: 0x%X\r\n\tCS: 0x%X\r\n\tEFLAGS: 0x%X\r\n\tSS: 0x%X\r\n"
        "\tDS: 0x%X\r\n",
        registers.edi, registers.esi, registers.ebp, registers.esp, registers.ebx, registers.edx, registers.ecx, registers.eax,
        registers.eip, registers.cs, registers.eflags, registers.ss,
        registers.ds);
    printStackTrace();
    nop();
}

void pageFaultHandler(int interrupt, int ec, isr_registers registers)
{
    extern PVOID getCR2();
    const char* action = NULLPTR;
    const char* mode = "kernel";
    BOOL isPresent = getBitFromBitfield(ec, 0);
    PVOID address = getCR2();
    if (getBitFromBitfield(ec, 4))
        action = "execute";
    else if (getBitFromBitfield(ec, 1) && !getBitFromBitfield(ec, 4))
        action = "write";
    else if (!getBitFromBitfield(ec, 1) && !getBitFromBitfield(ec, 4))
        action = "read";
    if (getBitFromBitfield(ec, 2))
        mode = "user";
    klog_error("Page fault in %s-mode, tid %d at %p while trying to %s a %s page, the address of said page being %p.\r\n",
        mode,
        GetTid(),
        (PVOID)registers.eip,
        action,
        isPresent ? "present" : "non-present",
        address);
    klog_error("Dumping registers...\r\n"
        "\tEDI: 0x%X\r\n\tESI: 0x%X\r\n\tEBP: 0x%X\r\n\tESP: 0x%X\r\n\tEBX: 0x%X\r\n\tEDX: 0x%X\r\n\tECX: 0x%X\r\n\tEAX: 0x%X\r\n"
        "\tEIP: 0x%X\r\n\tCS: 0x%X\r\n\tEFLAGS: 0x%X\r\n\tSS: 0x%X\r\n"
        "\tDS: 0x%X\r\n",
        registers.edi, registers.esi, registers.ebp, registers.esp, registers.ebx, registers.edx, registers.ecx, registers.eax,
        registers.eip, registers.cs, registers.eflags, registers.ss,
        registers.ds);
    printStackTrace();
    kpanic(NULLPTR, 0);
}

static void irq1Handler(int interrupt, isr_registers registers)
{
    (void)inb(0x60);
}
static void isr1Handler(int interrupt, int ec, isr_registers registers)
{
    return;
}

extern UINT32_T __attribute__((aligned(4096))) g_pageTable[1024][1024];

multiboot_info_t* g_multibootInfo = (multiboot_info_t*)0;

DWORD testThread(PVOID userData)
{
    UINTPTR_T parameters[2];
    SIZE_T size = 0;
    for (; ((PCHAR)userData)[size]; size++);
    parameters[0] = (UINTPTR_T)userData;
    parameters[1] = size;
    asm volatile("mov %0, %%ebx\r\n"
                 "mov $0, %%eax\r\n"
                 "int $0x40"
                 :
                 : "r"(parameters));
    return GetTid();
}

void kmain_thread()
{
    klog_info("The main thread's id is %d.\r\n", GetTid());

    InitializeInitRD((PVOID)(((multiboot_module_t*)g_multibootInfo->mods_addr)->mod_start));

    // Shutdowns the computer.
    onKernelPanic();

    klog_warning("Control flow reaches end of kmain.");
}

void kmain(multiboot_info_t* mbd, UINT32_T magic)
{
    g_multibootInfo = mbd;

    initGdt();
    initInterrupts();

    {
        int interrupt = 14;
        setExceptionHandlers(&interrupt, 1, pageFaultHandler);
    }

    InitializeTeriminal(TERMINALCOLOR_COLOR_WHITE | TERMINALCOLOR_COLOR_BLACK << 4);
    initAcpi();
    acpiEnable();

    setOnKernelPanic(onKernelPanic);

    kassert(magic == MULTIBOOT_BOOTLOADER_MAGIC, KSTR_LITERAL("Invalid magic number for multiboot.\r\n"));
    kassert(getBitFromBitfield(mbd->flags, 6), KSTR_LITERAL("No memory map provided from the bootloader.\r\n"));
    kassert(getBitFromBitfield(mbd->flags, 12), KSTR_LITERAL("No framebuffer info provided from the bootloader.\r\n"));
    kassert(mbd->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT, KSTR_LITERAL("The video type isn't MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT\r\n"));

    kmeminit();
    kInitializePaging();

    kinitserialports();
    
    if (InitSerialPort(COM1, MAKE_BAUDRATE_DIVISOR(115200), EIGHT_DATABITS, ONE_STOPBIT, PARITYBIT_EVEN, 2048) != 0)
        klog_warning("Could not initialize COM1.\r\n");
    if (InitSerialPort(COM2, MAKE_BAUDRATE_DIVISOR(115200), EIGHT_DATABITS, ONE_STOPBIT, PARITYBIT_EVEN, 2048) != 0)
        klog_warning("Could not initialize COM2.\r\n");

    /*extern SIZE_T g_kPhysicalMemorySize;
    extern SIZE_T g_kAvailablePhysicalMemorySize;
    extern SIZE_T g_kSystemPhysicalMemorySize;

    klog_info("%fmib of available memory, %fmib of reserved memory. %fmib total.\r\n",
        g_kAvailablePhysicalMemorySize / 1048576.0,
        g_kSystemPhysicalMemorySize / 1048576.0,
        g_kPhysicalMemorySize / 1048576.0);*/
    
    if (mbd->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME)
        klog_info("The bootloader's name is, \"%s.\"\r\n", (STRING)mbd->boot_loader_name);

    {
        int interrupt = 3;
        setExceptionHandlers(&interrupt, 1, testHandler);
        _int(3);
        kassert(interruptsWork, KSTR_LITERAL("Software interrupts do not work!\r\n"));
        setExceptionHandlers(&interrupt, 1, int3Handler);
        interrupt = 1;
        setPICInterruptHandlers(&interrupt, 1, irq1Handler);
        setExceptionHandlers(&interrupt, 1, isr1Handler);
    }

    extern void kinitmultitasking();
    kinitmultitasking();
    // control flow continues in kmain_thread.
    // If the control flow doesn't continue in kmain_thread:
    while (1)
    {
        hlt();
        kmain_thread();
    }
}