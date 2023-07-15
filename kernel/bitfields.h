/*
	bitfields.h

	Copyright (c) 2023 Omar Berrow
*/

#ifndef __OBOS_BITFIELDS_H
#define __OBOS_BITFIELDS_H

#include "types.h"

BOOL getBitFromBitfield(UINT32_T bitField, UINT32_T bit);
void setBitInBitfield(UINT32_T* bitField, UINT32_T bit);
void clearBitInBitfield(UINT32_T* bitField, UINT32_T bit);
BOOL getBitFromBitfieldBitmask(UINT32_T bitField, UINT32_T bitmask);
void setBitInBitfieldBitmask(UINT32_T* bitField, UINT32_T bitmask);
void clearBitInBitfieldBitmask(UINT32_T* bitField, UINT32_T bitmask);

UINT32_T generateBitfield(SIZE_T nBits, ...);
UINT32_T generateBitfieldBitmask(SIZE_T nBits, ...);

#endif