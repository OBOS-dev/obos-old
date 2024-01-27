/*
	oboskrnl/console.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>

#include <console.h>

#include <memory_manipulation.h>

#include <atomic.h>

#include <multitasking/arch.h>

#include <allocators/liballoc.h>
#include <allocators/vmm/vmm.h>

namespace obos
{	
	Console::Console(void* font, const con_framebuffer& output, bool lockCanUseMultitasking)
	{
		Initialize(font, output, lockCanUseMultitasking);
	}

	void Console::Initialize(void* font, const con_framebuffer& output, bool lockCanUseMultitasking)
	{
		if (m_modificationArray)
			delete[] m_modificationArray;
		m_font = (uint8_t*)font;
		m_framebuffer = output;
		m_nCharsHorizontal = output.width / 8;
		m_nCharsVertical = output.height / 16;
		m_lock.CanUseMultitasking(lockCanUseMultitasking);
		if (CanAllocateMemory())
		{
			m_modificationArray = new uint32_t[m_nCharsVertical];
			m_backbuffer.height = output.height;
			m_backbuffer.width = output.width;
			m_backbuffer.pitch = m_backbuffer.width * 4;
			m_backbuffer.addr = (uint32_t*)memory::VirtualAllocator{ nullptr }.VirtualAlloc(nullptr, m_backbuffer.width * m_backbuffer.height * sizeof(uint32_t), memory::PROT_NO_COW_ON_ALLOCATE);
			SetDrawBuffer(true);
		}
	}

	void Console::ConsoleOutput(const char* string)
	{
		m_lock.Lock();
		for(size_t i = 0; string[i]; __ImplConsoleOutputChar(string[i++], m_foregroundColour, m_backgroundColour, m_terminalX, m_terminalY, false));
		if (++m_nCallsSinceLastSwap >= maxCountsUntilSwap)
			SwapBuffers();
		m_lock.Unlock();
	}
	void Console::ConsoleOutput(const char* string, size_t size)
	{
		m_lock.Lock();
		for (size_t i = 0; i < size; __ImplConsoleOutputChar(string[i++], m_foregroundColour, m_backgroundColour, m_terminalX, m_terminalY, false));
		if (++m_nCallsSinceLastSwap >= maxCountsUntilSwap)
			SwapBuffers();
		m_lock.Unlock();
	}
	void Console::ConsoleOutput(char ch)
	{
		m_lock.Lock();
		__ImplConsoleOutputChar(ch, m_foregroundColour, m_backgroundColour, m_terminalX, m_terminalY);
		if ((m_nCallsSinceLastSwap += (ch == '\n')) >= maxCountsUntilSwap)
			SwapBuffers();
		m_lock.Unlock();
	}
	void Console::ConsoleOutput(char ch, uint32_t& x, uint32_t& y)
	{
		m_lock.Lock();
		__ImplConsoleOutputChar(ch, m_foregroundColour, m_backgroundColour, x, y);
		if ((m_nCallsSinceLastSwap += (ch == '\n')) >= maxCountsUntilSwap)
			SwapBuffers();
		m_lock.Unlock();
	}

	void Console::ConsoleOutput(char ch, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t& x, uint32_t& y)
	{
		m_lock.Lock();
		__ImplConsoleOutputChar(ch, foregroundColour, backgroundColour, x, y);
		if ((m_nCallsSinceLastSwap += (ch == '\n')) >= maxCountsUntilSwap)
			SwapBuffers();
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

	static void fillBackgroundTransparent(con_framebuffer& framebuffer, uint32_t backgroundColour, uint32_t initialBackgroundColour)
	{
		for (uint32_t i = 0; i < framebuffer.height * framebuffer.width; i++)
			framebuffer.addr[i] = (framebuffer.addr[i] == initialBackgroundColour) ? backgroundColour : framebuffer.addr[i];
	}

	void Console::SetColour(uint32_t foregroundColour, uint32_t backgroundColour, bool clearConsole)
	{
		if (clearConsole)
		{
			utils::dwMemset(m_drawingBuffer->addr, backgroundColour, static_cast<size_t>(m_drawingBuffer->height) * m_drawingBuffer->width);
			if (m_modificationArray)
				utils::dwMemset(m_modificationArray, 0xffffffff, m_nCharsVertical);
		}
		if (backgroundColour != m_backgroundColour && !clearConsole)
		{
			fillBackgroundTransparent(*m_drawingBuffer, backgroundColour, m_backgroundColour);
			if (m_modificationArray)
				utils::dwMemset(m_modificationArray, 0xffffffff, m_nCharsVertical);
		}
				
		m_foregroundColour = foregroundColour;
		m_backgroundColour = backgroundColour;
	}
	void Console::GetColour(uint32_t* foregroundColour, uint32_t* backgroundColour) const
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
		m_framebuffer = framebuffer;
	}
	void Console::GetFramebuffer(con_framebuffer* framebuffer) const
	{
		if (!framebuffer)
			return;
		framebuffer->addr = m_framebuffer.addr;
		framebuffer->width = m_framebuffer.width;
		framebuffer->height = m_framebuffer.height;
		framebuffer->pitch = m_framebuffer.pitch;
	}

	OBOS_EXPORT void Console::SetBackBuffer(const con_framebuffer& buffer)
	{
		if (buffer.height != m_framebuffer.height || buffer.width != m_framebuffer.width)
			return;
		if (buffer.addr == m_framebuffer.addr)
			return;
		m_backbuffer = buffer;
	}

	void Console::GetBackBuffer(con_framebuffer* buffer) const
	{
		if (!buffer)
			return;
		*buffer = m_backbuffer;
	}

	void Console::GetConsoleBounds(uint32_t* horizontal, uint32_t* vertical) const
	{
		if (horizontal)
			*horizontal = m_nCharsHorizontal;
		if(vertical)
			*vertical = m_nCharsVertical;
	}

	void Console::SwapBuffers()
	{
		m_nCallsSinceLastSwap = 0;
		if (!m_backbuffer.addr)
			return;
		if (m_framebuffer.lock)
			m_framebuffer.lock->Lock();
		if (!m_modificationArray)
		{
			if ((m_framebuffer.pitch / 4) != m_backbuffer.width)
			{
				// Oh come on!
				for (size_t y = 0; y < m_framebuffer.height; y++)
					utils::dwMemcpy(m_framebuffer.addr + y * m_framebuffer.pitch / 4, m_backbuffer.addr + y * m_backbuffer.width, m_framebuffer.width);
				if (m_framebuffer.lock)
					m_framebuffer.lock->Unlock();
				return;
			}
			utils::dwMemcpy(m_framebuffer.addr, m_backbuffer.addr, m_backbuffer.height * m_backbuffer.width);
			if (m_framebuffer.lock)
				m_framebuffer.lock->Unlock();
			return;
		}
		const size_t bitLength = (sizeof(*m_modificationArray) * 4);
		for (size_t ty = 0; ty < m_nCharsVertical; ty++)
		{
			if ((m_modificationArray[ty / bitLength] >> (ty % bitLength)) & 1)
			{
				// Copy the line.
				uint32_t cy = ty * 16 + ((size_t)(ty != ((size_t)m_nCharsVertical - 1)) * 4);
				for (uint32_t y = cy; y < (cy + 16); y++)
					utils::dwMemcpy(m_framebuffer.addr + y * m_framebuffer.pitch / 4, m_backbuffer.addr + y * m_backbuffer.width, m_framebuffer.width);
			}
		}
		if (m_framebuffer.lock)
			m_framebuffer.lock->Unlock();
		utils::dwMemset(m_modificationArray, 0, m_nCharsVertical);
	}
	void Console::SetDrawBuffer(bool isBackbuffer)
	{
		m_drawingBuffer = isBackbuffer ? &m_backbuffer : &m_framebuffer;
	}
	void Console::putChar(char ch, uint32_t x, uint32_t y, uint32_t fgcolor, uint32_t bgcolor)
	{
		int cy;
		int mask[8] = { 128,64,32,16,8,4,2,1 };
		const uint8_t* glyph = m_font + (int)ch * 16;
		y = y * 16 + 16;
		x <<= 3;
		if (x > m_drawingBuffer->width)
			x = 0;
		if (y >= m_drawingBuffer->height)
			y = m_drawingBuffer->height - 16;
		for (cy = 0; cy < 16; cy++) 
		{
			uint32_t realY = y + cy - 12;
			uint32_t* framebuffer = m_drawingBuffer->addr + realY * (m_drawingBuffer->pitch / 4);	
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
	void Console::newlineHandler(uint32_t& x, uint32_t& y)
	{
		x = 0;
		if (++y == m_nCharsVertical)
		{
			utils::dwMemset(m_drawingBuffer->addr, 0, (size_t)m_drawingBuffer->pitch / 4 * 16);
			utils::dwMemcpy(m_drawingBuffer->addr, m_drawingBuffer->addr + (m_drawingBuffer->pitch / 4 * 16), (size_t)m_drawingBuffer->pitch / 4 * ((size_t)m_drawingBuffer->height - 16));
			utils::dwMemset(m_drawingBuffer->addr + (size_t)m_drawingBuffer->pitch / 4 * ((size_t)m_drawingBuffer->height - 16), 0, (size_t)m_drawingBuffer->pitch / 4 * 16);
			utils::dwMemset(m_modificationArray, 0xffffffff, m_nCharsVertical);
			SwapBuffers();
			y--;
		}
	}
	void Console::__ImplConsoleOutputChar(char ch, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t& x, uint32_t& y, bool incrementCallsSinceSwap)
	{
		if (!m_drawingBuffer->addr)
			return;
		if (m_drawingBuffer->lock)
			m_drawingBuffer->lock->Lock();
		switch (ch)
		{
		case '\n':
			newlineHandler(x, y);
			m_nCallsSinceLastSwap += incrementCallsSinceSwap;
			if (m_nCallsSinceLastSwap >= maxCountsUntilSwap)
				SwapBuffers();
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
		case ' ':
			putChar(' ', ++x, y, foregroundColour, backgroundColour);
			break;
		default:
			if (x >= m_nCharsHorizontal)
				newlineHandler(x, y);
			putChar(ch, x++, y, foregroundColour, backgroundColour);
			break;
		}
		if (m_modificationArray)
			m_modificationArray[y / (sizeof(*m_modificationArray) * 4)] |= 1<<(y % (sizeof(*m_modificationArray) * 4));
		if (m_drawingBuffer->lock)
			m_drawingBuffer->lock->Unlock();
	}
}