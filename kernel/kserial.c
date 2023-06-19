#include "kserial.h"

#include "inline-asm.h"

typedef struct __serialPortData
{
    // First [inputBufferSize] bytes of the buffer will be the input buffer, and the rest [outputBufferSize] bytes will
    // be for the output buffer.
    char* buf;
    SIZE_T inputBufferSize;
    char*  inputBufferOffset;
    SIZE_T outputBufferSize;
    char*  outputBufferOffset;
    DWORD errorState;
    BOOL initalized : 1;
    UINT16_T baudRateDivisior;
    DATABITS dataBits;
    STOPBITS stopBits;
    PARITYBIT paritybit;
} serialPortData;
static serialPortData s_serialPortData[4];
BOOL zeroedSerialPortData = FALSE;

static SIZE_T getPortIndex(SERIALPORT port)
{
    int index = 0;
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
            return -1;
    }
    return index;
}

// https://wiki.osdev.org/Serial_Ports#Line_status_register
enum lineStatusRegister
{
    // Data ready
    LSTATUS_DR,
    // Overrun Error
    LSTATUS_OE,
    // Parity error
    LSTATUS_PE,
    // Framing error
    LSTATUS_FE,
    // Break indicator
    LSTATUS_BI,
    // Transmitter holding register empty (Data can now be sent)
    LSTATUS_THRE,
    // Transmitter empty
    LSTATUS_TEMT,
    // An impending error.
    LSTATUS_IMPENDING_ERROR
};

//static void comInterrupt(SERIALPORT port, enum lineStatusRegister lineStatus)
//{
//    switch (lineStatus)
//    {
//    case LSTATUS_DR:
//        break;
//    case LSTATUS_OE:
//        break;
//    case LSTATUS_PE:
//        break;
//    case LSTATUS_FE:
//        break;
//    case LSTATUS_BI:
//        break;
//    case LSTATUS_THRE:
//        break;
//    case LSTATUS_TEMT:
//        break;
//    case LSTATUS_IMPENDING_ERROR:    
//        break;
//    default:
//        break;
//    }
//}
//static void com1Com3ISR()
//{
//    // IRQ4 (COM1 and COM3)
//    /*const int irq = 4;
//    SERIALPORT port;
//    BYTE COM1_lineStatus = inb(COM1 + 5);
//    BYTE COM3_lineStatus = inb(COM3 + 5);*/
//    
//}

static void* memset(PVOID destination, CHAR data, SIZE_T size)
{
    PCHAR _dest = destination;
    for (int i = 0; i < size; i++)
        _dest[i] = data;
    return destination;
}

void kinitserialports()
{
    if (zeroedSerialPortData)
        return;
    serialPortData serialPortDatum;
    memset(&serialPortDatum, 0, sizeof(serialPortData));
    for(int i = 0; i < sizeof(s_serialPortData) / sizeof(serialPortData); i++)
        s_serialPortData[i] = serialPortDatum;
    zeroedSerialPortData = TRUE;
}

int InitSerialPort(
    SERIALPORT port,
    UINT16_T baudRateDivisior,
    DATABITS dataBits,
    STOPBITS stopBits,
    PARITYBIT paritybit,
    SIZE_T inputBufferSize,
    SIZE_T outputBufferSize)
{
    if(!zeroedSerialPortData)
        return 3;
    int index = getPortIndex(port);
    if  (port == -1)
        return 2;
    if (s_serialPortData[index].initalized)
        return 4;

    // Disable all interrupts
    outb(port + 1, 0x00);
    // Enable DLAB
    outb(port + 3, 0x80);
    // Set the baud rate divisor
    outb(port, baudRateDivisior & 0xFF);
    outb(port + 1, (baudRateDivisior & 0xFF00) >> 16);
    // Set the data bits, stop bits, and parity bit
    outb(port + 3, dataBits | stopBits | paritybit);

    // IRQs enabled, RTS/DSR are set.
    outb(port + 4, 0x0B);
    // Set the serial chip to "loopback mode,"
    outb(port + 4, 0x1E);

    // Check if the serial port is working correctly. 
    outb(port, 0xAE);
    if (inb(port) != 0xAE)
        return 1;
    
    // Set the serial chip to "normal operation mode."
    outb(port + 4, 0x0F);

    s_serialPortData[index].baudRateDivisior = baudRateDivisior;
    s_serialPortData[index].dataBits = dataBits;
    s_serialPortData[index].stopBits = stopBits;
    s_serialPortData[index].paritybit = paritybit;
    s_serialPortData[index].initalized = TRUE;
    // kheapalloc needs to be tested with multiple blocks.
    s_serialPortData[index].buf = 0x00;
    s_serialPortData[index].inputBufferOffset = 0x00;
    s_serialPortData[index].outputBufferOffset = 0x00;
    s_serialPortData[index].inputBufferSize = inputBufferSize;
    s_serialPortData[index].outputBufferSize = outputBufferSize;
    return 0;
}
int writeSerialPort(SERIALPORT port, CSTRING buf, SIZE_T sizeBuf)
{
    int index = getPortIndex(port);
    if(!s_serialPortData[index].initalized)
        return -1;
    //for(int i = 0; i < sizeBuf; i++)

    return 0;
}