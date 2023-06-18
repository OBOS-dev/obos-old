#include "klog.h"

#include "terminal.h"
#include "inline-asm.h"

static void defaultKernelPanic()
{
    while(1);
}

static void(*onKernelPanic)() = defaultKernelPanic;

void setOnKernelPanic(void (*callback)())
{
    onKernelPanic = callback;
}
void resetOnKernelPanic()
{
    onKernelPanic = defaultKernelPanic;
}

void kpanic(CSTRING message, SIZE_T size)
{
    if((void*)message) 
    {
        UINT8_T oldColor = TerminalGetColor();
        const char prefix[] = "Kernel panic! Message: \r\n";
        TerminalSetColor(TERMINALCOLOR_COLOR_RED | TERMINALCOLOR_COLOR_BLACK << 4);
        TerminalOutput(KSTR_LITERAL(prefix));
        TerminalOutput(message, size);
        TerminalSetColor(oldColor);
        for(int i = 0; prefix[i]; i++) { outb(0x3F8, prefix[i]);outb(0x2F8, prefix[i]); }
        for(int i = 0; message[i]; i++) { outb(0x3F8, message[i]); outb(0x2F8, message[i]); };
    }
    onKernelPanic();
}
void kassert(BOOL expression, CSTRING message, SIZE_T size)
{
    if(!expression)
        kpanic(message, size);
}

void klog_info(CSTRING message)
{
    UINT8_T oldColor = TerminalGetColor();
    const char prefix[] = "[KERNEL_INFO] ";
    TerminalSetColor(TERMINALCOLOR_COLOR_GREEN | TERMINALCOLOR_COLOR_BLACK << 4);
    TerminalOutput(KSTR_LITERAL(prefix));
    TerminalOutputString(message);
    TerminalSetColor(oldColor);
    for(int i = 0; prefix[i]; i++) { outb(0x3F8, prefix[i]);outb(0x2F8, prefix[i]); }
    for(int i = 0; message[i]; i++) { outb(0x3F8, message[i]); outb(0x2F8, message[i]); };
}
void klog_warning(CSTRING message)
{
    UINT8_T oldColor = TerminalGetColor();
    const char prefix[] = "[KERNEL_WARNING] ";
    TerminalSetColor(TERMINALCOLOR_COLOR_YELLOW | TERMINALCOLOR_COLOR_BLACK << 4);
    TerminalOutput(KSTR_LITERAL(prefix));
    TerminalOutputString(message);
    TerminalSetColor(oldColor);
    for(int i = 0; prefix[i]; i++) { outb(0x3F8, prefix[i]);outb(0x2F8, prefix[i]); }
    for(int i = 0; message[i]; i++) { outb(0x3F8, message[i]); outb(0x2F8, message[i]); };
}
void klog_error(CSTRING message)
{
    UINT8_T oldColor = TerminalGetColor();
    const char prefix[] = "[KERNEL_ERROR] ";
    TerminalSetColor(TERMINALCOLOR_COLOR_LIGHT_RED | TERMINALCOLOR_COLOR_BLACK << 4);
    TerminalOutput(KSTR_LITERAL(prefix));
    TerminalOutputString(message);
    TerminalSetColor(oldColor);
    for(int i = 0; prefix[i]; i++) { outb(0x3F8, prefix[i]);outb(0x2F8, prefix[i]); }
    for(int i = 0; message[i]; i++) { outb(0x3F8, message[i]); outb(0x2F8, message[i]); };
}