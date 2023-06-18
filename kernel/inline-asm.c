#include "inline-asm.h"
#include "types.h"

void outb(UINT16_T port, UINT8_T val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) :"memory");
}
void outw(UINT16_T port, UINT16_T val)
{
    asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) :"memory");
}
UINT8_T inb(UINT16_T port)
{
    volatile UINT8_T ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}
UINT16_T inw(UINT16_T port)
{
    volatile UINT16_T ret;
    asm volatile ( "inw %1, %0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}
void io_wait(void)
{
    outb(0x80, 0);
}