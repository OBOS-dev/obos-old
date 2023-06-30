#include "bitfields.h"

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