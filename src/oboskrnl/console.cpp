/*
	oboskrnl/console.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>

#include <console.h>

#include <memory_manipulation.h>

#include <atomic.h>

#include <multitasking/arch.h>

namespace obos
{	
	Console::Console(void* font, con_framebuffer output)
	{
		Initialize(font, output);
	}

	void Console::Initialize(void* font, con_framebuffer output)
	{
		m_font = (uint8_t*)font;
		m_framebuffer = output;
		m_nCharsHorizontal = output.width / 8;
		m_nCharsVertical = output.height / 16;
	}

	void Console::ConsoleOutput(const char* string)
	{
		uintptr_t val = thread::stopTimer();
		for(size_t i = 0; string[i]; ConsoleOutput(string[i++], m_foregroundColour, m_backgroundColour, m_terminalX, m_terminalY));
		thread::startTimer(val);
	}
	void Console::ConsoleOutput(const char* string, size_t size)
	{
		uintptr_t val = thread::stopTimer();
		for (size_t i = 0; i < size; ConsoleOutput(string[i++], m_foregroundColour, m_backgroundColour, m_terminalX, m_terminalY));
		thread::startTimer(val);
	}
	void Console::ConsoleOutput(char ch)
	{
		ConsoleOutput(ch, m_foregroundColour, m_backgroundColour, m_terminalX, m_terminalY);
	}
	void Console::ConsoleOutput(char ch, uint32_t& x, uint32_t& y)
	{
		ConsoleOutput(ch, m_foregroundColour, m_backgroundColour, x, y);
	}

	void Console::ConsoleOutput(char ch, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t& x, uint32_t& y)
	{
		if (!m_framebuffer.addr)
			return;
		m_lock.Lock();
		uintptr_t val = thread::stopTimer();
		switch (ch)
		{
		case '\n':
			newlineHandler(x,y);
			break;
		case '\r':
			x = 0;
			break;
		case '\t':
			x += 4 - (x % 4);
			break;
		case '\b':
			putChar(' ', --x, y, foregroundColour, backgroundColour);
			break;
		default:
			if (x >= m_nCharsHorizontal)
				newlineHandler(x,y);
			putChar(ch, x++, y, foregroundColour, backgroundColour);
			break;
		}
		thread::startTimer(val);
		m_lock.Unlock();
	}

	void Console::SetPosition(uint32_t x, uint32_t y)
	{
		m_terminalX = x;
		m_terminalY = y;
	}
	void Console::GetPosition(uint32_t* x, uint32_t* y)
	{
		if(x)
			*x = m_terminalX;
		if(y)
			*y = m_terminalY;
	}

	void fillBackgroundTransparent(con_framebuffer& framebuffer, uint32_t backgroundColour, uint32_t initialBackgroundColour)
	{
		for (uint32_t i = 0; i < framebuffer.height * framebuffer.width; i++)
			framebuffer.addr[i] = (framebuffer.addr[i] == initialBackgroundColour) ? backgroundColour : framebuffer.addr[i];
	}

	void Console::SetColour(uint32_t foregroundColour, uint32_t backgroundColour, bool clearConsole)
	{
		if (backgroundColour != m_backgroundColour)
			if (!clearConsole)
				fillBackgroundTransparent(m_framebuffer, backgroundColour, m_backgroundColour);
			else
				utils::dwMemset(m_framebuffer.addr, backgroundColour, static_cast<size_t>(m_framebuffer.height) * m_framebuffer.width);
		else {}
		m_foregroundColour = foregroundColour;
		m_backgroundColour = backgroundColour;
	}
	void Console::GetColour(uint32_t* foregroundColour, uint32_t* backgroundColour)
	{
		if (foregroundColour)
			*foregroundColour = m_foregroundColour;
		if (backgroundColour)
			*backgroundColour = m_backgroundColour;
	}

	void Console::SetFont(uint8_t* font)
	{
		m_font = font;
	}
	void Console::GetFont(uint8_t** font)
	{
		if(font)
			*font = m_font;
	}

	void Console::SetFramebuffer(con_framebuffer framebuffer)
	{
		m_framebuffer.addr = framebuffer.addr;
		m_framebuffer.width = framebuffer.width;
		m_framebuffer.height = framebuffer.height;
		m_framebuffer.pitch = framebuffer.pitch;
		m_nCharsHorizontal = framebuffer.width / 8;
		m_nCharsVertical = framebuffer.height / 16;
	}
	void Console::GetFramebuffer(con_framebuffer* framebuffer)
	{
		if (!framebuffer)
			return;
		framebuffer->addr = m_framebuffer.addr;
		framebuffer->width = m_framebuffer.width;
		framebuffer->height = m_framebuffer.height;
		framebuffer->pitch = m_framebuffer.pitch;
	}

	void Console::GetConsoleBounds(uint32_t* horizontal, uint32_t* vertical)
	{
		if (horizontal)
		*horizontal = m_nCharsHorizontal;
		if(vertical)
			*vertical = m_nCharsVertical;
	}


	void Console::CopyFrom(con_framebuffer* buffer)
	{
		if (buffer->height != m_framebuffer.height || buffer->width != m_framebuffer.width)
			return;
		utils::dwMemcpy(m_framebuffer.addr, buffer->addr, m_framebuffer.width * m_framebuffer.height);
	}

#pragma GCC push_options
#pragma GCC optimize("O3")
	void Console::putChar(char ch, uint32_t x, uint32_t y, uint32_t fgcolor, uint32_t bgcolor)
	{
		int cy;
		int mask[8] = { 128,64,32,16,8,4,2,1 };
		const uint8_t* glyph = m_font + (int)ch * 16;
		y = y * 16 + 16;
		x <<= 3;
		if (x > m_framebuffer.width)
			x = 0;
		if (y > m_framebuffer.height)
			y = 0;
		for (cy = 0; cy < 16; cy++) 
		{
			uint32_t realY = y + cy - 12;
			uint32_t* framebuffer = m_framebuffer.addr + realY * (m_framebuffer.pitch / 4);
			framebuffer[x + 0] = (glyph[cy] & mask[0]) ? fgcolor : bgcolor;
			framebuffer[x + 1] = (glyph[cy] & mask[1]) ? fgcolor : bgcolor;
			framebuffer[x + 2] = (glyph[cy] & mask[2]) ? fgcolor : bgcolor;
			framebuffer[x + 3] = (glyph[cy] & mask[3]) ? fgcolor : bgcolor;
			framebuffer[x + 4] = (glyph[cy] & mask[4]) ? fgcolor : bgcolor;
			framebuffer[x + 5] = (glyph[cy] & mask[5]) ? fgcolor : bgcolor;
			framebuffer[x + 6] = (glyph[cy] & mask[6]) ? fgcolor : bgcolor;
			framebuffer[x + 7] = (glyph[cy] & mask[7]) ? fgcolor : bgcolor;
		}
	}
#pragma GCC pop_options

	void Console::newlineHandler(uint32_t& x, uint32_t& y)
	{
		x = 0;
		if (++y == m_nCharsVertical)
		{
			utils::dwMemset(m_framebuffer.addr, 0, static_cast<size_t>(m_framebuffer.pitch) * 16 / 4);
			utils::dwMemcpy(m_framebuffer.addr, m_framebuffer.addr + m_framebuffer.pitch * 16 / 4, ((m_framebuffer.pitch / 4) * m_framebuffer.height * 4) - m_framebuffer.pitch * 16 / 4);
			utils::dwMemset((uint32_t*)(reinterpret_cast<uintptr_t>(m_framebuffer.addr) + (m_framebuffer.pitch * m_framebuffer.height) - m_framebuffer.pitch * 16),
				m_backgroundColour, m_framebuffer.pitch * 16 / 4);
			y--;
		}
	}
}