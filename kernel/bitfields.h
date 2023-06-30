#ifndef __OBOS_BITFIELDS_H
#define __OBOS_BITFIELDS_H

#include "types.h"

BOOL getBitFromBitfield(UINT32_T bitField, UINT32_T bit);
void setBitInBitfield(UINT32_T* bitField, UINT32_T bit);
void clearBitInBitfield(UINT32_T* bitField, UINT32_T bit);

#endif