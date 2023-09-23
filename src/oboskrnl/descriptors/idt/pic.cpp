/*
	pic.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <descriptors/idt/pic.h>

#include <inline-asm.h>

#include <utils/bitfields.h>

#define ICW1_ICW4	0x01		/* Indicates that ICW4 will be present */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10

namespace obos
{
	Pic::Pic(UINT16_T outputPortCommand, UINT16_T outputPortData)
	{
		setPorts(outputPortCommand, outputPortData);
	}
	void Pic::setPorts(UINT16_T outputPortCommand, UINT16_T outputPortData)
	{
		m_outputPortCommand = outputPortCommand;
		m_outputPortData = outputPortData;
		m_setPorts = true;
	}

	Pic& Pic::sendCommandByte(UINT8_T byte)
	{
		if (!m_setPorts)
			return *this;
		outb(m_outputPortCommand, byte);
		io_wait();
		return *this;
	}
	Pic& Pic::sendCommandWord(UINT16_T word)
	{
		if (!m_setPorts)
			return *this;
		outw(m_outputPortCommand, word);
		io_wait();
		return *this;
	}
	Pic& Pic::sendDataByte(UINT8_T byte)
	{
		if (!m_setPorts)
			return *this;
		outb(m_outputPortData, byte);
		io_wait();
		return *this;
	}
	Pic& Pic::sendDataWord(UINT16_T word)
	{
		if (!m_setPorts)
			return *this;
		outw(m_outputPortData, word);
		io_wait();
		return *this;
	}

	UINT8_T Pic::recvCommandByte()
	{
		if (!m_setPorts)
			return 0;
		return inb(m_outputPortCommand);
	}
	UINT16_T Pic::recvCommandWord()
	{
		if (!m_setPorts)
			return 0;
		return inw(m_outputPortCommand);
	}
	UINT8_T Pic::recvDataByte()
	{
		if (!m_setPorts)
			return 0;
		return inb(m_outputPortData);
	}
	UINT16_T Pic::recvDataWord()
	{
		if (!m_setPorts)
			return 0;
		return inw(m_outputPortData);
	}

	Pic& Pic::sendEOI()
	{
		sendCommandByte(s_picEOI);
		return *this;
	}

	Pic& Pic::remap(BYTE startInterrupt, BYTE cascadeIdentity)
	{
		sendCommandByte(ICW1_INIT | ICW1_ICW4);
		// Legends say the pic was still waiting for the initialization words to this day...
		/*sendCommandByte(startInterrupt);
		sendCommandByte(cascadeIdentity);
		sendCommandByte(ICW4_8086);*/
		sendDataByte(startInterrupt);
		sendDataByte(cascadeIdentity);
		sendDataByte(ICW4_8086);
		return *this;
	}

	Pic& Pic::enableIrq(BYTE interrupt)
	{
		if (interrupt > 7)
			interrupt -= 8;
		utils::RawBitfield bitfield = recvDataByte();
		utils::clearBitInBitfield(bitfield, interrupt);
		sendDataByte(bitfield);
		return *this;
	}
	Pic& Pic::disableIrq(BYTE interrupt)
	{
		if (interrupt > 7)
			interrupt -= 8;
		utils::RawBitfield bitfield = recvDataByte();
		utils::setBitInBitfield(bitfield, interrupt);
		sendDataByte(bitfield);
		return *this;
	}

	Pic& Pic::disable()
	{
		sendDataByte(0xFF);
		return *this;
	}

	bool Pic::issuedInterrupt(UINT8_T interrupt)
	{
		if (interrupt > 7)
			interrupt -= 8;
		sendCommandByte(0x0b);
		UINT8_T isr_register = recvCommandByte();
		return utils::testBitInBitfield((UINT32_T)isr_register, interrupt);
	}

	void SendEOI(BYTE irqNumber)
	{
		if (irqNumber > 15)
			return;
		Pic masterPic{ obos::Pic::PIC1_CMD, obos::Pic::PIC1_DATA };
		if (irqNumber < 7 && masterPic.issuedInterrupt(irqNumber))
			masterPic.sendEOI();
		if (irqNumber > 7)
		{
			masterPic.setPorts(obos::Pic::PIC2_CMD, obos::Pic::PIC2_DATA);
			// masterPic is now slavePic.
			if (masterPic.issuedInterrupt(irqNumber))
				masterPic.sendEOI();
		}
	}
}