#include "kserial.h"

#include "inline-asm.h"

int InitSerialPort(
    SERIALPORT port,
    UINT16_T baudRateDivisior,
    DATABITS dataBits,
    STOPBITS stopBits,
    PARITYBIT paritybit)
{
    // Disable all interrupts
    outb(port + 1, 0x00);
    outb(port + 3, 0x80);
    // Set the baud rate divisor
    outb(port, baudRateDivisior & 0xFF);
    outb(port + 1, (baudRateDivisior & 0xFF00) >> 16);
    // Set the data bits, stop bits, and parity bit
    outb(port + 3, dataBits | stopBits | paritybit);

    return 0;
}