/*
	oboskrnl/console.cpp

	Copyright (c) 2023 Omar Berrow
*/

#ifdef __INTELLISENSE__
#undef __STDC_HOSTED__
#endif
#include <int.h>

#include <console.h>

#include <memory_manipulation.h>

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
		m_nCharsHorizontal = output.width / 16;
		m_nCharsVertical = output.height / 8;
	}

	void Console::ConsoleOutput(const char* string)
	{
		for(size_t i = 0; string[i]; ConsoleOutput(string[i++], m_foregroundColour, m_backgroundColour, m_terminalX, m_terminalY));
	}
	void Console::ConsoleOutput(const char* string, size_t size)
	{
		for (size_t i = 0; i < size; ConsoleOutput(string[i++], m_foregroundColour, m_backgroundColour, m_terminalX, m_terminalY));
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
		switch (ch)
		{
		case '\n':
			y++;
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
			putChar(ch, x++, y, foregroundColour, backgroundColour);
			break;
		}
	}

	void Console::SetPosition(uint32_t x, uint32_t y)
	{
		m_terminalX = x;
		m_terminalY = y;
	}
	void Console::GetPosition(uint32_t* x, uint32_t* y)
	{
		*x = m_terminalX;
		*y = m_terminalY;
	}

	void Console::SetColour(uint32_t foregroundColour, uint32_t backgroundColour)
	{
		m_foregroundColour = foregroundColour;
		m_backgroundColour = backgroundColour;
	}
	void Console::GetColour(uint32_t* foregroundColour, uint32_t* backgroundColour)
	{
		*foregroundColour = m_foregroundColour;
		*backgroundColour = m_backgroundColour;
	}

	void Console::SetFont(uint8_t* font)
	{
		m_font = font;
	}
	void Console::GetFont(uint8_t** font)
	{
		*font = m_font;
	}

	void Console::SetFramebuffer(con_framebuffer framebuffer)
	{
		m_framebuffer.addr = framebuffer.addr;
		m_framebuffer.width = framebuffer.width;
		m_framebuffer.height = framebuffer.height;
	}
	void Console::GetFramebuffer(con_framebuffer* framebuffer)
	{
		framebuffer->addr = m_framebuffer.addr;
		framebuffer->width = m_framebuffer.width;
		framebuffer->height = m_framebuffer.height;
	}

	void Console::GetConsoleBounds(uint32_t* horizontal, uint32_t* vertical)
	{
		*horizontal = m_nCharsHorizontal;
		*vertical = m_nCharsVertical;
	}


	void Console::plotPixel(uint32_t color, uint32_t x, uint32_t y)
	{
		m_framebuffer.addr[y * m_framebuffer.width + x] = color;
	}
	void Console::putChar(char ch, uint32_t x, uint32_t y, uint32_t fgcolor, uint32_t bgcolor)
	{
		int cx, cy;
		int mask[8] = { 128,64,32,16,8,4,2,1 };
		const uint8_t* glyph = m_font + (int)ch * 16;
		if (x > m_nCharsHorizontal)
			x = 0;
		if (y > m_framebuffer.height)
			y = 0;
		x <<= 3;
		y = y * 16 + 16;

		for (cy = 0; cy < 16; cy++) {
			for (cx = 0; cx < 8; cx++) {
				plotPixel(glyph[cy] & mask[cx] ? fgcolor : bgcolor, x + cx, y + cy - 12);
			}
		}
	}
}