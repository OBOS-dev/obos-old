#ifndef __OBOS_KSERIAL_H
#define __OBOS_KSERIAL_H

#include "types.h"

#define MAKE_BAUDRATE(x) ((int)(115200 / x))

typedef enum __serialPort
{
    COM1 = 0x3F8,
    COM2 = 0x2F8,
    COM3 = 0x3E8,
    COM4 = 0x2E8
} SERIALPORT;
typedef enum __parityBit
{
    PARITYBIT_NONE,
    PARITYBIT_ODD = 0b10000,
    PARITYBIT_EVEN = 0b11000,
    PARITYBIT_MARK = 0b10100,
    PARITYBIT_SPACE = 0b11100,
} PARITYBIT;
typedef enum __dataBits
{
    FIVE_DATABITS,
    SIX_DATABITS,
    SEVEN_DATABITS,
    EIGHT_DATABITS,
} DATABITS;
typedef enum __stopBits
{
    ONE_STOPBIT,
    ONE_HALF_STOPBIT = 0b0100,
    TWO_STOPBIT = 0b0100
} STOPBITS;

void kinitserialports();

int InitSerialPort(
    SERIALPORT port,
    UINT16_T baudRateDivisior,
    DATABITS dataBits,
    STOPBITS stopBits,
    PARITYBIT paritybit,
    SIZE_T inputBufferSize,
    SIZE_T outputBufferSize);
int writeSerialPort(SERIALPORT port, CSTRING buf, SIZE_T sizeBuf);
#endif