/*
	bitfields.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <utils/bitfields.h>

namespace obos
{
	namespace utils
	{
		// class BitfieldBase

		BitfieldBase::BitfieldBase()
		{
			m_bitfield = 0;
		}

		void BitfieldBase::setBit(UINTPTR_T bitmask)
		{
			m_bitfield |= bitmask;
		}
		void BitfieldBase::clearBit(UINTPTR_T bitmask)
		{
			m_bitfield &= ~(bitmask);
		}
		bool BitfieldBase::getBit(UINTPTR_T bitmask) const
		{
			return (m_bitfield & bitmask) == bitmask;
		}

		bool BitfieldBase::operator[](UINTPTR_T bitmask) const
		{
			return getBit(bitmask);
		}

		BitfieldBase::operator UINTPTR_T()
		{
			return m_bitfield;
		}
		BitfieldBase::operator bool()
		{
			return m_bitfield == 0xFFFFFFFF;
		}

		// class Bitfield

		Bitfield::Bitfield()
			:BitfieldBase()
		{
		}

		void Bitfield::setBit(UINT8_T bit)
		{
			if (bit > 31)
				return;
			BitfieldBase::setBit(BITFIELD_FROM_BIT(bit));
		}
		void Bitfield::clearBit(UINT8_T bit)
		{
			if (bit > 31)
				return;
			BitfieldBase::clearBit(BITFIELD_FROM_BIT(bit));
		}
		bool Bitfield::getBit(UINT8_T bit) const
		{
			if (bit > 31)
				return BitfieldBase::getBit(BITFIELD_FROM_BIT(31));
			return BitfieldBase::getBit(BITFIELD_FROM_BIT(bit));
		}

		bool Bitfield::operator[](UINT8_T bit) const
		{
			return getBit(bit);
		}

		Bitfield::operator bool()
		{
			return BitfieldBase::operator bool();
		}

		// IntegerBitfield

		IntegerBitfield::IntegerBitfield()
			:Bitfield()
		{
		}

		IntegerBitfield::IntegerBitfield(UINTPTR_T bitfield)
		{
			m_bitfield = bitfield;
		}

		void IntegerBitfield::setBitfield(UINTPTR_T newBitfield)
		{
			m_bitfield = newBitfield;
		}
		
		void setBitInBitfield(UINT32_T& bitfield, UINT8_T bit)
		{
			bitfield |= (1 << bit);
		}
		void clearBitInBitfield(UINT32_T& bitfield, UINT8_T bit)
		{
			bitfield &= ~(static_cast<UINT32_T>(1) << bit);
		}
		bool testBitInBitfield(const UINT32_T bitfield, UINT8_T bit)
		{
			if (!bitfield)
				return false;
			UINTPTR_T val = bitfield >> bit;
			return val & 0b1;
		}

#ifdef __x86_64__
		void setBitInBitfield(UINT64_T& bitfield, UINT8_T bit)
		{
			bitfield |= (static_cast<UINT64_T>(1) << bit);
		}
		void clearBitInBitfield(UINT64_T& bitfield, UINT8_T bit)
		{
			bitfield &= ~(static_cast<UINT64_T>(1) << bit);
		}
		bool testBitInBitfield(const UINT64_T bitfield, UINT8_T bit)
		{
			if (!bitfield)
				return false;
			const UINT64_T val = bitfield >> bit;
			return val & 0b1;
		}
#endif
	}
}