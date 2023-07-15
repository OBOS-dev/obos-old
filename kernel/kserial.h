/*
    kserial.h

    Copyright (c) 2023 Omar Berrow
*/

#ifndef __OBOS_KSERIAL_H
#define __OBOS_KSERIAL_H

#include "types.h"

#define MAKE_BAUDRATE_DIVISOR(x) ((int)(115200 / x))
#define MAKE_BAUDRATE_FROM_DIVISOR(x) ((int)(115200 * x))

typedef enum __serialPort
{
    COM1 = 0x03F8,
    COM2 = 0x02F8,
    COM3 = 0x03E8,
    COM4 = 0x02E8
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

/// <summary>
/// Do not use in user-mode please.
/// </summary>
void kinitserialports();

/// <summary>
/// Checks if the specified serial port was initialized with InitSerialPort. If it was not, it does not mean the serial port does not work.
/// </summary>
/// <param name="port"> - The serial port.</param>
/// <returns>-1 if the specified port was invalid, otherwise a boolean.</returns>
INT IsSerialPortInitialized(SERIALPORT port);
/// <summary>
/// Gets the last error for the specified serial port. This is not the error code returned from any functions that manipulate the serial port, instead
/// it is from the line status.
/// </summary>
/// <param name="port"> - The serial port.</param>
/// <returns>-1 if the specified port was invalid, otherwise the last error.</returns>
DWORD GetSerialPortError(SERIALPORT port);
/// <summary>
/// Clears the last error for the specified serial port.
/// It is recommended to call this function if you are going to check the error code of a serial function.
/// If the specified port is invalid, the function will silently fail.
/// </summary>
/// <param name="port"> - The serial port.</param>
void ClearSerialPortError(SERIALPORT port);
/// <summary>
/// Initializes the specified serial port.
/// </summary>
/// <param name="port"> - The port to initialize.</param>
/// <param name="baudRateDivisor"> - The divisor of the baud rate. You can get this with the macro, "MAKE_BAUDRATE_DIVISOR."</param>
/// <param name="dataBits"> - The amount of data bits.</param>
/// <param name="stopBits"> - The amount of stop bits.</param>
/// <param name="paritybit"> - What type of parity bit to use.</param>
/// <param name="inputBufferSize"> - The size of the input buffer. This cannot be zero. If this buffer gets full, data will be discarded when the isr gets called.</param>
/// <returns> * 0 on success.<para/>
///           * 1 if testing the serial port failed.<para/>
///           * 2 if the specified port is invalid.<para/>
///           * 3 if kinitserialports() wasn't called by the kernel while booting the operating system.<para/>
///           * 4 if the serial port was already initialzed.</returns>
INT InitSerialPort(
    SERIALPORT port,
    UINT16_T baudRateDivisor,
    DATABITS dataBits,
    STOPBITS stopBits,
    PARITYBIT paritybit,
    SIZE_T inputBufferSize);
/// <summary>
/// Writes the serial port at the specified port. This function is not thread-safe.
/// </summary>
/// <param name="port"> - The serial port to write to.</param>
/// <param name="buf"> - The buffer to write.</param>
/// <param name="sizeBuf"> - The size of the buffer.</param>
/// <returns> * -1 if the serial port wasn't initialzed.<para/>
///           *  0 on success.</returns>
INT WriteSerialPort(SERIALPORT port, CSTRING buf, SIZE_T sizeBuf);
/// <summary>
/// Reads from the serial port at the specified port. This function is asynchronous, but not thread-safe.
/// </summary>
/// <param name="port"> - The serial port to write to.</param>
/// <param name="outbuf"> - The buffer to read into.</param>
/// <param name="bytesToRead"> - The amount of bytes to read.</param>
/// <returns> * -1 if the port wasn't initialized, otherwise the amount of bytes read.
/// <para>
///     If the amount of bytes read is zero, there is no data to be read. If there is not
///     enough bytes in the input buffer, the output buffer will be truncated to the amount of bytes, and that will be returned. If there is enough bytes, it will 
///     read [bytesToRead] bytes from the buffer and return [bytesToRead]
/// </para>
/// </returns>
INT ReadSerialPort(SERIALPORT port, STRING outbuf, SIZE_T bytesToRead);
#endif