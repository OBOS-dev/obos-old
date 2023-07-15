/*
    bitfields.c

    Copyright (c) 2023 Omar Berrow
*/

#include "bitfields.h"

#include <stdarg.h>

BOOL getBitFromBitfield(UINT32_T bitField, UINT32_T bit)
{
    const UINT32_T bitmask = (1 << bit);
    return (bitField & bitmask) == bitmask;
}
void setBitInBitfield(UINT32_T* bitField, UINT32_T bit)
{
    *bitField |= (1 << bit);
}
void clearBitInBitfield(UINT32_T* bitField, UINT32_T bit)
{
    *bitField &= (1 << bit);
}
BOOL getBitFromBitfieldBitmask(UINT32_T bitField, UINT32_T bitmask)
{
    return (bitField & bitmask) == bitmask;
}
void setBitInBitfieldBitmask(UINT32_T* bitField, UINT32_T bitmask)
{
    (*bitField) |= bitmask;
}
void clearBitInBitfieldBitmask(UINT32_T* bitField, UINT32_T bitmask)
{
    (*bitField) &= bitmask;
}

UINT32_T generateBitfield(SIZE_T nBits, ...)
{
    va_list list; va_start(list, nBits);
    UINT32_T ret = 0;
    for (int i = 0; i < nBits; i++)
        ret |= (1 << va_arg(list, UINT32_T));
    va_end(list);
    return ret;
}
UINT32_T generateBitfieldBitmask(SIZE_T nBits, ...)
{
    va_list list; va_start(list, nBits);
    UINT32_T ret = 0;
    for (int i = 0; i < nBits; i++)
        ret |= va_arg(list, UINT32_T);
    va_end(list);
    return ret;
}
