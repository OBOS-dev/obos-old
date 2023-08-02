/*
	bitfields.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <utils/bitfields.h>

static UINT32_T makeBitmask(UINT8_T base)
{
	return 1 << base;
}

namespace obos
{
	namespace utils
	{
		// class BitfieldBase

		BitfieldBase::BitfieldBase()
		{
			m_bitfield = 0;
		}

		void BitfieldBase::setBit(UINT32_T bitmask)
		{
			m_bitfield |= bitmask;
		}
		void BitfieldBase::clearBit(UINT32_T bitmask)
		{
			m_bitfield &= ~(bitmask);
		}
		bool BitfieldBase::getBit(UINT32_T bitmask) const
		{
			return (m_bitfield & bitmask) == bitmask;
		}

		bool BitfieldBase::operator[](UINT32_T bitmask) const
		{
			return getBit(bitmask);
		}

		BitfieldBase::operator UINT32_T()
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
			BitfieldBase::setBit(makeBitmask(bit));
		}
		void Bitfield::clearBit(UINT8_T bit)
		{
			if (bit > 31)
				return;
			BitfieldBase::clearBit(makeBitmask(bit));
		}
		bool Bitfield::getBit(UINT8_T bit) const
		{
			if (bit > 31)
				return BitfieldBase::getBit(makeBitmask(31));
			return BitfieldBase::getBit(makeBitmask(bit));
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

		IntegerBitfield::IntegerBitfield(UINT32_T bitfield)
		{
			m_bitfield = bitfield;
		}

		void IntegerBitfield::setBitfield(UINT32_T newBitfield)
		{
			m_bitfield = newBitfield;
		}
		
		// Bare functions. Use if polymorphism isn't working correctly.

		void setBitInBitfield(UINT32_T& bitfield, UINT8_T bit)
		{
			bitfield |= (1 << bit);
		}
		void clearBitInBitfield(UINT32_T& bitfield, UINT8_T bit)
		{
			bitfield &= ~(1 << bit);
		}
		bool testBitInBitfield(const UINT32_T& bitfield, UINT8_T bit)
		{
			const UINT32_T bitmask = (1 << bit);
			return (bitfield & bitmask) == bitmask;
		}
	}
}