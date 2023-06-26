#ifndef __OBOS_KSERIAL_H
#define __OBOS_KSERIAL_H

#include "types.h"

#define MAKE_BAUDRATE_DIVISOR(x) ((int)(115200 / x))
#define MAKE_BAUDRATE_FROM_DIVISOR(x) ((int)(115200 * x))

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

// Returns zero if it 'port' is not initalized, one if it is, and -1 if 'port' has an invalid value.
INT IsSerialPortInitialized(SERIALPORT port);
// Returns -1 on error. Otherwise it returns the last error for the serial port.
DWORD GetSerialPortError(SERIALPORT port);
// Returns zero on success, one if loopback isn't working, two if 'port' has an invalid value, three if "kinitserialports" wasn't called (kernel dev only!), and four if the serial
// port was already initialized.
INT InitSerialPort(
    SERIALPORT port,
    UINT16_T baudRateDivisior,
    DATABITS dataBits,
    STOPBITS stopBits,
    PARITYBIT paritybit,
    SIZE_T inputBufferSize,
    SIZE_T outputBufferSize);
INT writeSerialPort(SERIALPORT port, CSTRING buf, SIZE_T sizeBuf);
#endif