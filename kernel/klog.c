/*
    klog.c

    Copyright (c) 2023 Omar Berrow
*/

#include "klog.h"

#include "terminal.h"
#include "inline-asm.h"
#include "kserial.h"
#include "interrupts.h"
#include "kserial.h"

#include "multiboot.h"

static BOOL resetTimerInitialized = FALSE;
static INT irqIterations = 0;

static void resetComputerInterrupt(int interrupt, isr_registers registers)
{
    if (!resetTimerInitialized)
        return;
    resetTimerInitialized = irqIterations++ != 500;
    if (!resetTimerInitialized)
    {
        int irq0 = 0;
        resetPICInterruptHandlers(&irq0, 1);

        while (inb(0x64) == 0b10);
        outb(0x64, 0xFE);
        while (1);
    }
}

static void defaultKernelPanic()
{
    klog_info("Forcefully resetting the computer in ten seconds.\r\n");
    {
        int _irq0 = 0;
        setPICInterruptHandlers(&_irq0, 1, resetComputerInterrupt);

        UINT32_T divisor = 1193180 / 50;

        outb(0x43, 0x36);

        UINT8_T l = (UINT8_T)(divisor & 0xFF);
        UINT8_T h = (UINT8_T)((divisor >> 8) & 0xFF);

        // Send the frequency divisor.
        outb(0x40, l);
        outb(0x40, h);
        resetTimerInitialized = TRUE;
        sti();
    }
    while (resetTimerInitialized)
        asm volatile ("hlt");
}

static void(*onKernelPanic)() = defaultKernelPanic;

// Credit: http://www.strudel.org.uk/itoa/
static char* itoa(int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, * ptr1 = result, tmp_char;
    int tmp_value;

    do 
    {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
    } while (value);

    // Apply negative sign
    if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr) 
    {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

static void strreverse(char* begin, int size)
{
    int i = 0;
    char tmp = 0;

    for (i = 0; i < size / 2; i++)
    {
        tmp = begin[i];
        begin[i] = begin[size - i - 1];
        begin[size - i - 1] = tmp;
    }
}
// Credit: http://www.strudel.org.uk/itoa/
// 'buf' should be at least 32 bytes (depends on the base, for decimal 12 bytes).
char* itoa_unsigned(unsigned int value, char* result, int base) 
{
    // check that the base if valid

    if (base < 2 || base > 16)
    { 
        *result = 0;
        return result; 
    }

    char* out = result;

    int quotient = value;

    do
    {

        int abs = (quotient % base) < 0 ? (-(quotient % base)) : (quotient % base);

        *out = "0123456789abcdef"[abs];

        ++out;

        quotient /= base;

    } while (quotient);

    // Only apply negative sign for base 10

    if (value < 0 && base == 10) 
        *out++ = '-';

    strreverse(result, out - result);

    return result;

}
#define MAX_PRECISION	(10)
static const double rounders[MAX_PRECISION + 1] =
{
    0.5,				// 0
    0.05,				// 1
    0.005,				// 2
    0.0005,				// 3
    0.00005,			// 4
    0.000005,			// 5
    0.0000005,			// 6
    0.00000005,			// 7
    0.000000005,		// 8
    0.0000000005,		// 9
    0.00000000005		// 10
};

char* ftoa(double f, char* buf, int precision)
{
    char* ptr = buf;
    char* p = ptr;
    char* p1;
    char c;
    long intPart;

    // check precision bounds
    if (precision > MAX_PRECISION)
        precision = MAX_PRECISION;

    // sign stuff
    if (f < 0)
    {
        f = -f;
        *ptr++ = '-';
    }

    if (precision < 0)  // negative precision == automatic precision guess
    {
        if (f < 1.0) precision = 6;
        else if (f < 10.0) precision = 5;
        else if (f < 100.0) precision = 4;
        else if (f < 1000.0) precision = 3;
        else if (f < 10000.0) precision = 2;
        else if (f < 100000.0) precision = 1;
        else precision = 0;
    }

    // round value according the precision
    if (precision)
        f += rounders[precision];

    // integer part...
    intPart = f;
    f -= intPart;

    if (!intPart)
        *ptr++ = '0';
    else
    {
        // save start pointer
        p = ptr;

        // convert (reverse order)
        while (intPart)
        {
            *p++ = '0' + intPart % 10;
            intPart /= 10;
        }

        // save end pos
        p1 = p;

        // reverse result
        while (p > ptr)
        {
            c = *--p;
            *p = *ptr;
            *ptr++ = c;
        }

        // restore end pos
        ptr = p1;
    }

    // decimal part
    if (precision)
    {
        // place decimal point
        *ptr++ = '.';

        // convert
        while (precision--)
        {
            f *= 10.0;
            c = f;
            *ptr++ = '0' + c;
            f -= c;
        }
    }

    // terminating zero
    *ptr = 0;

    return buf;
}

// Simplified version of vprintf
static void _vprintf(CSTRING format, va_list list, void(*character_print_callback)(char ch), void(*string_print_callback)(CSTRING string))
{
    BOOL wasPercent = FALSE;
    for (int i = 0; format[i]; i++)
    {
        if (wasPercent)
        {
            wasPercent = FALSE;
            continue;
        }
        char ch = format[i];
        if (ch == '%')
        {
            ch = format[i + 1];
            wasPercent = TRUE;
            switch (ch)
            {
                // A 32-bit integer.
            case 'i':
            case 'd':
            {
                char buf[13] = { 0 };
                int value = va_arg(list, int);
                itoa(value, buf, 10);
                string_print_callback(buf);
                break;
            }
            // An unsigned 32-bit integer.
            case 'u':
            {
                char buf[12] = { 0 };
                unsigned int value = va_arg(list, unsigned int);
                itoa_unsigned(value, buf, 10);
                string_print_callback(buf);
                break;
            }
            // An unsigned 32-bit integer (but printed in octal).
            case 'o':
            {
                char buf[10] = { 0 };
                unsigned int value = va_arg(list, unsigned int);
                itoa_unsigned(value, buf, 8);
                string_print_callback(buf);
            }
            // An unsigned 32-bit integer (but printed in lowercase hex).
            case 'x':
            {
                char buf[9] = { 0 };
                unsigned int value = va_arg(list, unsigned int);
                itoa_unsigned(value, buf, 16);
                string_print_callback(buf);
                break;
            }
            // An unsigned 32-bit integer (but printed in uppercase hex).
            case 'X':
            {
                char buf[9] = { 0 };
                unsigned int value = va_arg(list, unsigned int);
                itoa_unsigned(value, buf, 16);
                // Make the string uppercase
                for (int i = 0; buf[i]; i++)
                    if (buf[i] >= 'a' && buf[i] <= 'z')
                        buf[i] = (buf[i] - 'a') + 'A';
                string_print_callback(buf);
                break;
            }
            // A floating-point number.
            case 'f':
            {
                char buf[65] = { 0 };
                double val = va_arg(list, double);
                ftoa(val, buf, MAX_PRECISION);
                string_print_callback(buf);
                break;
            }
            // A pointer.
            case 'p':
            {
                char buf[9] = { 0 };
                UINTPTR_T value = va_arg(list, UINTPTR_T);
                if (value == 0)
                {
                    string_print_callback("(nil)");
                    break;
                }
                itoa_unsigned(value, buf, 16);
                // Make the string uppercase
                for (int i = 0; buf[i]; i++)
                    if (buf[i] >= 'a' && buf[i] <= 'z')
                        buf[i] = (buf[i] - 'a') + 'A';
                string_print_callback("0x");
                string_print_callback(buf);
                break;
            }
            // A character.
            case 'c':
            {
                char ch = va_arg(list, int);
                character_print_callback(ch);
                break;
            }
            // A string of characters.
            case 's':
            {
                char* str = va_arg(list, char*);
                string_print_callback(str);
                break;
            }
            // A percentage sign.
            case '%':
            {
                character_print_callback('%');
                break;
            }
            default:
                break;
            }
            continue;
        }
        character_print_callback(ch);
    }
}

void setOnKernelPanic(void (*callback)())
{
    onKernelPanic = callback;
}
void resetOnKernelPanic()
{
    onKernelPanic = defaultKernelPanic;
}

static void callback1_terminal(char ch)
{
    TerminalOutputCharacter(ch);
}
static void callback2_terminal(CSTRING string)
{
    TerminalOutputString(string);
}
static void callback1_klog(char ch)
{
    TerminalOutputCharacter(ch);
    WriteSerialPort(COM1, &ch, 1);
    WriteSerialPort(COM2, &ch, 1);
}
static void callback2_klog(CSTRING string)
{
    for (int i = 0; string[i]; i++)
        callback1_klog(string[i]);
}

static void kpanic_printf(CSTRING format, ...)
{
    va_list list = NULLPTR; va_start(list, format);
    _vprintf(format, list, callback1_klog, callback2_klog);
    va_end(list);
}

void printStackTrace()
{
    callback2_klog("Stack trace: \r\n");
    struct stackframe {
        struct stackframe* ebp;
        UINTPTR_T eip;
    } *stk;
    asm("movl %%ebp,%0" : "=r"(stk) ::);
    int nFrames;
    for (nFrames = 0; stk; nFrames++, stk = stk->ebp);
    asm("movl %%ebp,%0" : "=r"(stk) ::);
    kpanic_printf("\t%u: %p\r\n", nFrames, printStackTrace);
    for (int frame = nFrames - 1; stk && nFrames >= 0; frame--)
    {
        kpanic_printf("\t%u: %p\r\n", frame, stk->eip);
        stk = stk->ebp;
    }
}
void kpanic(CSTRING message, SIZE_T size)
{
    if ((void*)message && message[0])
    {
        UINT8_T oldColor = TerminalGetColor();
        const char prefix[] = "Kernel panic! Message: \r\n";
        TerminalSetColor(TERMINALCOLOR_COLOR_RED | TERMINALCOLOR_COLOR_BLACK << 4);
        callback2_klog(prefix);
        callback2_klog(message);
        printStackTrace();
        TerminalSetColor(oldColor);
    }
    onKernelPanic();
}
void kassert(BOOL expression, CSTRING message, SIZE_T size)
{
    if(!expression)
        kpanic(message, size);
}

void  attribute(format(printf, 1, 2)) printf(CSTRING format, ...)
{
    va_list list = NULLPTR;
    va_start(list, format);
    _vprintf(format, list, callback1_terminal, callback2_terminal);
    va_end(list);
}
void vprintf(CSTRING format, va_list list)
{
    _vprintf(format, list, callback1_terminal, callback2_terminal);
}

void attribute(format(printf, 1, 2)) klog_info(CSTRING format, ...)
{
    UINT8_T oldColor = TerminalGetColor();
    const char prefix[] = "[KERNEL_INFO] ";
    TerminalSetColor(TERMINALCOLOR_COLOR_GREEN | TERMINALCOLOR_COLOR_BLACK << 4);
    
    callback2_klog(prefix);

    va_list list = NULLPTR;
    va_start(list, format);
    _vprintf(format, list, callback1_klog, callback2_klog);
    va_end(list);

    TerminalSetColor(oldColor);
}
void attribute(format(printf, 1, 2)) klog_warning(CSTRING format, ...)
{
    UINT8_T oldColor = TerminalGetColor();
    const char prefix[] = "[KERNEL_WARNING] ";
    TerminalSetColor(TERMINALCOLOR_COLOR_YELLOW | TERMINALCOLOR_COLOR_BLACK << 4);
    
    callback2_klog(prefix);

    va_list list = NULLPTR;
    va_start(list, format);
    _vprintf(format, list, callback1_klog, callback2_klog);
    va_end(list);

    TerminalSetColor(oldColor);
}
void  attribute(format(printf, 1, 2)) klog_error(CSTRING format, ...)
{
    UINT8_T oldColor = TerminalGetColor();
    const char prefix[] = "[KERNEL_ERROR] ";
    TerminalSetColor(TERMINALCOLOR_COLOR_LIGHT_RED | TERMINALCOLOR_COLOR_BLACK << 4);
    
    callback2_klog(prefix);

    va_list list = NULLPTR;
    va_start(list, format);
    _vprintf(format, list, callback1_klog, callback2_klog);
    va_end(list);

    TerminalSetColor(oldColor);
}