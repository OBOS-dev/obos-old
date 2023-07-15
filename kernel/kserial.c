/*
    kserial.c

    Copyright (c) 2023 Omar Berrow
*/

#include "kserial.h"

#include "inline-asm.h"
#include "interrupts.h"
#include "bitfields.h"
#include "liballoc/liballoc_1_1.h"

typedef struct __serialPortData
{
    // First [inputBufferSize] bytes of the buffer will be the input buffer, and the rest [outputBufferSize] bytes will
    // be for the output buffer.
    char* buf;
    SIZE_T inputBufferSize;
    SIZE_T inputBufferCount;
    char*  inputBufferOffset;
    DWORD errorState;
    BOOL initalized : 1;
    UINT16_T baudRateDivisor;
    DATABITS dataBits;
    STOPBITS stopBits;
    PARITYBIT paritybit;
} serialPortData;
static serialPortData s_serialPortData[4];
BOOL zeroedSerialPortData = FALSE;

static SIZE_T getPortIndex(SERIALPORT port)
{
    INT index = 0;
    switch(port)
    {
        case COM1:
            index = 0;
            break;
        case COM2:
           index = 1;
            break;
        case COM3:
            index = 2;
            break;
        case COM4:
            index = 3;
            break;
        default:
            index = -1;
    }
    return index;
}

// https://wiki.osdev.org/Serial_Ports#Line_status_register
enum lineStatusRegister
{
    // Data ready
    LSTATUS_DR = 1,
    // Overrun Error
    LSTATUS_OE = 2,
    // Parity error
    LSTATUS_PE = 4,
    // Framing error
    LSTATUS_FE = 8,
    // Break indicator
    LSTATUS_BI = 16,
    // Transmitter holding register empty (Data can now be sent)
    LSTATUS_THRE = 32,
    // Transmitter empty
    LSTATUS_TEMT = 64,
    // An impending error.
    LSTATUS_IMPENDING_ERROR = 128
};

static void comInterrupt(SERIALPORT port, enum lineStatusRegister lineStatus)
{
    int index = getPortIndex(port);
    if (getBitFromBitfield(lineStatus, 0) && !getBitFromBitfield(lineStatus, 7))
    {
        *(s_serialPortData[index].inputBufferOffset) = inb(port);
        s_serialPortData[index].inputBufferOffset++;
        s_serialPortData[index].inputBufferCount++;
    }
    if (getBitFromBitfield(lineStatus, 1))
        s_serialPortData[index].errorState |= 32;
    if (getBitFromBitfield(lineStatus, 2))
        s_serialPortData[index].errorState |= 64;
    if (getBitFromBitfield(lineStatus, 3))
        s_serialPortData[index].errorState |= 128;
    if (getBitFromBitfield(lineStatus, 4)); // Not implemented.
    if (getBitFromBitfield(lineStatus, 5) || getBitFromBitfield(lineStatus, 6)); // Not needed.
    if (getBitFromBitfield(lineStatus, 7))
        s_serialPortData[index].errorState |= 256;
}
static void com1Com3ISR(int interrupt, isr_registers registers)
{
    // IRQ4 (COM1 and COM3)
    BYTE COM1_lineStatus = inb(COM1 + 5);
    BYTE COM3_lineStatus = inb(COM3 + 5);
    if (COM1_lineStatus != 0 && IsSerialPortInitialized(COM1))
        comInterrupt(COM1, COM1_lineStatus);
    else
        (void)inb(COM1);
    if (COM3_lineStatus != 0 && IsSerialPortInitialized(COM3))
        comInterrupt(COM3, COM3_lineStatus);
    else
        (void)inb(COM3);
    
}
static void com2Com4ISR(int interrupt, isr_registers registers)
{
    // IRQ3 (COM2 and COM4)
    BYTE COM2_lineStatus = inb(COM2 + 5);
    BYTE COM4_lineStatus = inb(COM4 + 5);
    if (COM2_lineStatus != 0 && IsSerialPortInitialized(COM2))
        comInterrupt(COM2, COM2_lineStatus);
    else
        (void)inb(COM2);
    if (COM4_lineStatus != 0 && IsSerialPortInitialized(COM4))
        comInterrupt(COM4, COM4_lineStatus);
    else
        (void)inb(COM4);
}

static void* memset(PVOID destination, CHAR data, SIZE_T size)
{
    PCHAR _dest = destination;
    for (INT i = 0; i < size; i++)
        _dest[i] = data;
    return destination;
}

void kinitserialports()
{
    if (zeroedSerialPortData)
        return;
    serialPortData serialPortDatum;

    memset(&serialPortDatum, 0, sizeof(serialPortData));
    for(INT i = 0; i < sizeof(s_serialPortData) / sizeof(serialPortData); i++)
        s_serialPortData[i] = serialPortDatum;
    zeroedSerialPortData = TRUE;

    int interrupt = 3;
    setPICInterruptHandlers(&interrupt, 1, com2Com4ISR);
    interrupt = 4;
    setPICInterruptHandlers(&interrupt, 1, com1Com3ISR);
}

INT IsSerialPortInitialized(SERIALPORT port)
{
    int index = getPortIndex(port);
    if (index == -1)
        return -1;
    return s_serialPortData[index].initalized;
}
DWORD GetSerialPortError(SERIALPORT port)
{
    int index = getPortIndex(port);
    if (index == -1)
        return -1;
    return s_serialPortData[index].errorState;
}
void ClearSerialPortError(SERIALPORT port)
{
    int index = getPortIndex(port);
    if (index == -1)
        return;
    s_serialPortData[index].errorState = 0;
}
INT InitSerialPort(
    SERIALPORT port,
    UINT16_T baudRateDivisor,
    DATABITS dataBits,
    STOPBITS stopBits,
    PARITYBIT paritybit,
    SIZE_T inputBufferSize)
{
    if(!zeroedSerialPortData)
        return 3;
    INT index = getPortIndex(port);
    if (index == -1)
        return 2;
    if (s_serialPortData[index].initalized)
    {
        s_serialPortData[index].errorState = 4;
        return 4;
    }

    // Make sure interrupts are enabled.
    outb(port + 1, 0x01);
    // Enable DLAB
    outb(port + 3, 0x80);
    // Set the baud rate divisor
    outb(port, baudRateDivisor & 0xFF);
    outb(port + 1, (baudRateDivisor & 0xFF00) >> 16);
    // Set the data bits, stop bits, and parity bit
    outb(port + 3, dataBits | stopBits | paritybit);

    // IRQs enabled, RTS/DSR are set.
    outb(port + 4, 0x0B);
    // Set the serial chip to "loopback mode,"
    outb(port + 4, 0x1E);

    // Check if the serial port is working correctly. 
    outb(port, 0xAE);
    if (inb(port) != 0xAE)
    {
        s_serialPortData[index].errorState = 1;
        return 1;
    }
    
    // Enable all interrupts for the serial port.
    outb(port + 2, 0x0F);

    // Exit loopback mode.
    outb(port + 4, 0x0B);

    switch (port)
    {
    case COM1:
    case COM3:
        enablePICInterrupt(4);
        break;
    case COM2:
    case COM4:
        enablePICInterrupt(3);
        break;
    default:
        break;
    }

    s_serialPortData[index].baudRateDivisor = baudRateDivisor;
    s_serialPortData[index].dataBits = dataBits;
    s_serialPortData[index].stopBits = stopBits;
    s_serialPortData[index].paritybit = paritybit;
    s_serialPortData[index].initalized = TRUE;
    s_serialPortData[index].errorState = 0;
    s_serialPortData[index].buf = kcalloc(inputBufferSize, sizeof(char));
    s_serialPortData[index].inputBufferOffset = s_serialPortData[index].buf;
    s_serialPortData[index].inputBufferSize = inputBufferSize;
    return 0;
}
INT WriteSerialPort(SERIALPORT port, CSTRING buf, SIZE_T sizeBuf)
{
    INT index = getPortIndex(port);
    if(!s_serialPortData[index].initalized)
        return -1;
    for (int i = 0; i < sizeBuf; i++)
    {
        while (!getBitFromBitfield(inb(port + 5), 5));
        outb(port, buf[i]);
    }
    return 0;
}
INT ReadSerialPort(SERIALPORT port, STRING outbuf, SIZE_T bytesToRead)
{
    int index = getPortIndex(port);
    if (index == -1)
        return -1;
    int bytesInBuffer = s_serialPortData[index].inputBufferCount;
    if (!bytesInBuffer)
        return 0;
    disablePICInterrupt(port == COM2 || port == COM4 ? 3 : 4);
    PCHAR currentCharacter = s_serialPortData[index].buf;
    int i;
    s_serialPortData[index].inputBufferOffset = currentCharacter;
    for (i = 0; i < bytesToRead; i++, currentCharacter++)
    {
        if (i > bytesInBuffer)
            break;
        s_serialPortData[index].inputBufferCount--;
        outbuf[i] = *(currentCharacter);
        *currentCharacter = 0;
    }
    enablePICInterrupt(port == COM2 || port == COM4 ? 3 : 4);
    return i;
}
