/*
	bitfields.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

#define BITFIELD_FROM_BIT(x) (1 << x)

namespace obos
{
	namespace utils
	{
		// Bare functions.

		void setBitInBitfield(UINT32_T& bitfield, UINT8_T bit);
		void clearBitInBitfield(UINT32_T& bitfield, UINT8_T bit);
		bool testBitInBitfield(const UINT32_T bitfield, UINT8_T bit);

#ifdef __x86_64__
		void setBitInBitfield(UINT64_T& bitfield, UINT8_T bit);
		void clearBitInBitfield(UINT64_T& bitfield, UINT8_T bit);
		bool testBitInBitfield(const UINT64_T bitfield, UINT8_T bit);
#endif

		class BitfieldBase
		{
		public:
			BitfieldBase();

			virtual void setBit(UINTPTR_T bitmask);
			virtual void clearBit(UINTPTR_T bitmask);
			virtual bool getBit(UINTPTR_T bitmask) const;

			virtual bool operator[](UINTPTR_T bitmask) const;

			virtual explicit operator UINTPTR_T();
			virtual operator bool();
		protected:
			UINTPTR_T m_bitfield;
		};
		class Bitfield : public BitfieldBase
		{
		public:
			Bitfield();

			virtual void setBit(UINT8_T bit);
			virtual void clearBit(UINT8_T bit);
			virtual bool getBit(UINT8_T bit) const;

			virtual bool operator[](UINT8_T bitmask) const;

			virtual operator bool();
		};
		class IntegerBitfield : public Bitfield
		{
		public:
			IntegerBitfield();
			IntegerBitfield(UINTPTR_T bitfield);

			void setBitfield(UINTPTR_T newBitfield);
		};
	
		using BitfieldBitmask = BitfieldBase;
		using RawBitfield = UINT32_T;
		using RawBitfield64 = UINT64_T;
	}
}