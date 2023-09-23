/*
	pic.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <types.h>

namespace obos
{
	class Pic final
	{
	public:
		inline static constexpr UINT16_T PIC1_CMD = 0x20;
		inline static constexpr UINT16_T PIC1_DATA = 0x21;
		inline static constexpr UINT16_T PIC2_CMD =  0xA0;
		inline static constexpr UINT16_T PIC2_DATA = 0xA1;

		Pic() = delete;
		Pic(UINT16_T outputPortCommand, UINT16_T outputPortData);

		void setPorts(UINT16_T outputPortCommand, UINT16_T outputPortData);

		Pic& sendCommandByte(UINT8_T byte);
		Pic& sendCommandWord(UINT16_T word);
		Pic& sendDataByte(UINT8_T byte);
		Pic& sendDataWord(UINT16_T word);

		UINT8_T recvCommandByte();
		UINT16_T recvCommandWord();
		UINT8_T recvDataByte();
		UINT16_T recvDataWord();
		
		Pic& sendEOI();
		
		// Maps the interrupts 0-7 on the pic to [startInterrupt]-[startInterrupt + 7] on the cpu. Make sure these all have an entry in the idt, or you'll have a bad time.
		Pic& remap(BYTE startInterrupt, BYTE cascadeIdentity);

		Pic& enableIrq (BYTE interrupt);
		Pic& disableIrq(BYTE interrupt);

		Pic& disable();

		bool issuedInterrupt(UINT8_T interrupt);
		
		static inline constexpr UINT8_T s_picEOI = 0x20;
	private:
		UINT16_T m_outputPortCommand = 0;
		UINT16_T m_outputPortData = 0;
		bool m_setPorts = false;

	};
	void SendEOI(BYTE irqNumber);
}