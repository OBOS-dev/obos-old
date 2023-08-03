/*
	console.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <boot/multiboot.h>
#include <console.h>
#include <inline-asm.h>

#define UpdateCursorPosition() SetTerminalCursorPosition(s_terminalColumn, s_terminalRow)

namespace obos
{
	// Global variables.
	
	static BYTE s_consoleColor = 0;
	static DWORD s_terminalColumn = 0;
	static DWORD s_terminalRow = 0;
	static bool s_reachedEndTerminal;
	static UINT16_T* s_framebuffer = nullptr;
	static constexpr SIZE_T s_framebufferWidth  = 80;
	static constexpr SIZE_T s_framebufferHeight = 25;
	extern multiboot_info_t* g_multibootInfo;
	
	// Internal functions.
	
	static void memset(void* block, char val, SIZE_T size)
	{
		for (SIZE_T i = 0; i < size; ((PBYTE)block)[i++] = val);
	}
#define vga_entry(uc, color) ((UINT16_T) uc | (UINT16_T) color << 8)
	static void putcharAt(DWORD x, DWORD y, BYTE character, BYTE color)
	{
		if (x > s_framebufferWidth || y > s_framebufferHeight)
			return;
		s_framebuffer[y * s_framebufferWidth + x] = vga_entry(character, color);
	}
#undef vga_entry
	static void on_newline()
	{
		if (++s_terminalRow >= s_framebufferHeight)
		{
			s_reachedEndTerminal = true;
			memset(s_framebuffer, 0, (s_framebufferWidth - 1) * sizeof(UINT16_T));
			for (DWORD y = 0, y2 = 1; y < s_framebufferHeight; y++, y2++)
				for (DWORD x = 0; x < s_framebufferWidth; x++)
					s_framebuffer[y * s_framebufferWidth + x] = s_framebuffer[y2 * s_framebufferWidth + x];
			s_terminalRow = s_terminalRow - 1;
		}
		DWORD x = s_terminalColumn;
		DWORD y = 0;
		if (!s_reachedEndTerminal)
			y = s_terminalRow;
		else
			s_terminalRow = y = s_framebufferHeight - 1;
		SetTerminalCursorPosition(x,y);
	}

	// API

	void InitializeConsole(ConsoleColor foreground, ConsoleColor background)
	{
		if ((g_multibootInfo->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO) == MULTIBOOT_INFO_FRAMEBUFFER_INFO)
			s_framebuffer = (UINT16_T*)g_multibootInfo->framebuffer_addr;
		else
			s_framebuffer = (UINT16_T*)0xB8000; // Assume the frame buffer is at 0xB8000, which it probably it is at.
		s_terminalColumn = 0;
		s_terminalRow = 0;
		SetConsoleColor(foreground, background);
		for (SIZE_T y = 0; y < s_framebufferHeight; y++)
			for (SIZE_T x = 0; x < s_framebufferWidth; x++)
				putcharAt(x,y, ' ', s_consoleColor);
		SetCursor(true);
		UpdateCursorPosition();
	}
	void SetCursor(bool status)
	{
		if(status)
		{ 
			outb(0x3D4, 0x0A);
			outb(0x3D5, (inb(0x3D5) & 0xC0) | 7);

			outb(0x3D4, 0x0B);
			outb(0x3D5, (inb(0x3D5) & 0xE0) | 9);
		}
		else
		{
			outb(0x3D4, 0x0A);
			outb(0x3D5, 0x20);
		}
	}
	void SetConsoleColor(ConsoleColor foreground, ConsoleColor background)
	{
		s_consoleColor = (BYTE)foreground | (BYTE)background << 4;
	}
	void GetConsoleColor(ConsoleColor& foreground, ConsoleColor& background)
	{
		foreground = (ConsoleColor)(s_consoleColor & 0x0F);
		background = (ConsoleColor)(s_consoleColor >> 4);
	}
	void ConsoleOutput(CSTRING message, SIZE_T size)
	{
		for (SIZE_T i = 0; i < size; ConsoleOutputCharacter(message[i++]));
	}
	void ConsoleOutputString(CSTRING message)
	{
		for (SIZE_T i = 0; message[i]; ConsoleOutputCharacter(message[i++]));
	}
	void ConsoleOutputCharacter(CHAR ch)
	{
		switch (ch)
		{
		case '\n':
		{
			on_newline();
			break;
		}
		case '\r':
		{
			s_terminalColumn = 0;
			break;
		}
		case '\t':
		{
			DWORD x = ((s_terminalColumn >> 2) + 1) << 2;
			DWORD y = 0;
			if (!s_reachedEndTerminal)
				y = s_terminalRow;
			else
				y = s_framebufferHeight - 1;
			SetTerminalCursorPosition(x,y);
			for (DWORD i = 0; i < x && s_terminalColumn < s_framebufferWidth; i++)
				putcharAt(++s_terminalColumn, s_terminalRow, ' ', s_consoleColor);
			break;
		}
		case '\b':
		{
			if (s_terminalColumn == 0)
			// TODO: Make a beep here.
				break;
			putcharAt(--s_terminalColumn, s_terminalRow, ' ', s_consoleColor);
			DWORD x = s_terminalColumn;
			DWORD y = 0;
			if (!s_reachedEndTerminal)
				y = s_terminalRow;
			else
				y = s_framebufferHeight - 1;
			SetTerminalCursorPosition(x,y);
			return;
			}
		case '\0':
			break;
		default:
		{
			if (++s_terminalColumn >= s_framebufferWidth)
			{
				s_terminalColumn = 0;
				on_newline();
			}
			putcharAt(s_terminalColumn - 1, s_terminalRow, ch, s_consoleColor);
			UpdateCursorPosition();
		}
		}
	}
	void SetTerminalCursorPosition(DWORD x, DWORD y)
	{
		UINT16_T pos = y * s_framebufferWidth + x;

		outb(0x3D4, 0x0F);
		outb(0x3D5, (UINT8_T)(pos & 0xFF));
		outb(0x3D4, 0x0E);
		outb(0x3D5, (UINT8_T)((pos >> 8) & 0xFF));

		s_terminalColumn = x;
		s_terminalRow = y;
	}
}
