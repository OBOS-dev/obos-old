/*
	bitfields.h

	Copyright (c) 2023 Omar Berrow
*/

#include <types.h>

#define GET_FUNC_ADDR(f) ((UINTPTR_T)(f))

namespace obos
{
	namespace utils
	{
		void setBitInBitfield(UINT32_T& bitfield, UINT8_T bit);
		void clearBitInBitfield(UINT32_T& bitfield, UINT8_T bit);
		bool testBitInBitfield(const UINT32_T& bitfield, UINT8_T bit);

		class BitfieldBase
		{
		public:
			BitfieldBase();

			virtual void setBit(UINT32_T bitmask);
			virtual void clearBit(UINT32_T bitmask);
			virtual bool getBit(UINT32_T bitmask) const;

			virtual bool operator[](UINT32_T bitmask) const;

			explicit virtual operator UINT32_T();
			virtual operator bool();
		protected:
			UINT32_T m_bitfield;
		};
		class Bitfield : public BitfieldBase
		{
		public:
			Bitfield();

			virtual void setBit(UINT8_T bit);
			virtual void clearBit(UINT8_T bit);
			virtual bool getBit(UINT8_T bit) const;

			virtual bool operator[](UINT8_T bit) const;

			virtual operator bool();
		};
		class IntegerBitfield final : public Bitfield
		{
		public:
			IntegerBitfield();
			IntegerBitfield(UINT32_T bitfield);

			void setBitfield(UINT32_T newBitfield);
		};
	
		using BitfieldBitmask = BitfieldBase;
	}
}