/*
	console.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <boot/multiboot.h>
#include <console.h>
#include <inline-asm.h>

#include <utils/memory.h>

#include <memory_manager/paging/allocate.h>
#include <memory_manager/physical.h>

#include <multitasking/mutex/mutexHandle.h>

#define UpdateCursorPosition() SetTerminalCursorPosition(s_terminalColumn, s_terminalRow)
extern void strreverse(char* begin, int size);

namespace obos
{
	// Global variables.
	
	UINT32_T s_foregroundColor;
	UINT32_T s_backgroundColor;
	DWORD s_terminalColumn = 0;
	DWORD s_terminalRow = 0;
	bool s_reachedEndTerminal;
	UINT32_T* s_framebuffer = nullptr;
	UINT32_T* s_backbuffer = nullptr;
	SIZE_T s_framebufferWidth  = 0;
	SIZE_T s_framebufferHeight = 0;
	SIZE_T s_nCharsVertical = 0;
	SIZE_T s_nCharsHorizontal = 0;
	extern multiboot_info_t* g_multibootInfo;
	PBYTE font = nullptr;
	static multitasking::MutexHandle* s_consoleMutex;

	// Internal functions.
	
	static void memset(void* block, char val, SIZE_T size)
	{
		for (SIZE_T i = 0; i < size; ((PBYTE)block)[i++] = val);
	}
//#define vga_entry(uc, color) ((UINT16_T) uc | (UINT16_T) color << 8)
//	static void putcharAt(DWORD x, DWORD y, BYTE character, BYTE color)
//	{
//		if (x > s_framebufferWidth || y > s_framebufferHeight)
//			return;
//		s_framebuffer[y * s_framebufferWidth + x] = vga_entry(character, color);
//	}
//#undef vga_entry
	static void putPixel(UINT32_T color, DWORD x, DWORD y)
	{
		s_backbuffer[s_framebufferWidth * y + x] = color;
	}
	static void putcharAt(DWORD x, DWORD y, BYTE character, UINT32_T fgcolor, UINT32_T bgcolor)
	{
		int cx, cy;
		int mask[8] = { 128,64,32,16,8,4,2,1 };
		const unsigned char* glyph = font + (int)character * 16;

		x <<= 3;
		y += 16;

		for (cy = 0; cy < 16; cy++) {
			for (cx = 0; cx < 8; cx++) {
				putPixel(glyph[cy] & mask[cx] ? fgcolor : bgcolor, x + cx, y + cy - 12);
			}
		}
	}
	static void on_newline()
	{
		if (++s_terminalRow >= s_nCharsVertical)
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

	void InitializeConsole(UINT32_T foregroundColor, UINT32_T backgroundColor)
	{
		EnterKernelSection();
		s_framebuffer = (UINT32_T*)0xFFCFF000;
		s_terminalColumn = 0;
		s_terminalRow = 0;
		s_framebufferHeight = g_multibootInfo->framebuffer_height;
		s_framebufferWidth = g_multibootInfo->framebuffer_width;
		s_nCharsVertical = s_framebufferHeight / 16;
		s_nCharsHorizontal = s_framebufferWidth / 8;
		if (!s_consoleMutex)
			s_consoleMutex = new multitasking::MutexHandle{};
		if(!s_backbuffer)
		{
			SIZE_T size = (s_framebufferWidth * s_framebufferHeight * sizeof(DWORD));
			//0x900000;
			s_backbuffer = (UINT32_T*)memory::VirtualAlloc(s_backbuffer, size >> 12, memory::VirtualAllocFlags::WRITE_ENABLED);
		}
		s_consoleMutex->Lock();
		utils::memzero(s_backbuffer, s_framebufferWidth * s_framebufferHeight * 4);
		font = (PBYTE)(((multiboot_module_t*)g_multibootInfo->mods_addr)->mod_start);
		SetConsoleColor(foregroundColor, backgroundColor);
		utils::dwMemset(s_framebuffer, s_backgroundColor, s_framebufferWidth * s_framebufferHeight);
		s_consoleMutex->Unlock();
		SetCursor(true);
		UpdateCursorPosition();
		LeaveKernelSection();
	}
	void SetCursor(bool status)
	{
		/*if(status)
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
		}*/
	}
	void SetConsoleColor(UINT32_T foregroundColor, UINT32_T backgroundColor)
	{
		s_foregroundColor = foregroundColor; 
		s_backgroundColor = backgroundColor;
	}
	void GetConsoleColor(UINT32_T& foreground, UINT32_T& background)
	{
		foreground = s_foregroundColor;
		background = s_backgroundColor;
	}
	static void __Impl_swapBuffers();

	static void __Impl_OutputCharacter(char ch)
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
			SetTerminalCursorPosition(x, y);
			for (DWORD i = 0; i < x && s_terminalColumn < s_framebufferWidth; i++)
				putcharAt(++s_terminalColumn, s_terminalRow * 16, ' ', s_foregroundColor, s_backgroundColor);
			break;
		}
		case '\b':
		{
			if (s_terminalColumn == 0)
				// TODO: Make a beep here.
				break;
			putcharAt(--s_terminalColumn, s_terminalRow * 16, ' ', s_foregroundColor, s_backgroundColor);
			DWORD x = s_terminalColumn;
			DWORD y = 0;
			if (!s_reachedEndTerminal)
				y = s_terminalRow;
			else
				y = s_framebufferHeight - 1;
			SetTerminalCursorPosition(x, y);
			return;
		}
		case '\0':
			break;
		case ' ':
			s_terminalColumn++;
			break;
		default:
		{
			if (++s_terminalColumn >= s_nCharsHorizontal)
			{
				s_terminalColumn = 0;
				on_newline();
			}
			putcharAt(s_terminalColumn - 1, s_terminalRow * 16, ch, s_foregroundColor, s_backgroundColor);
			UpdateCursorPosition();
		}
		}
	}
	void ConsoleOutput(CSTRING message, SIZE_T size, bool _swapBuffers)
	{
		if (!s_consoleMutex)
			return;
		EnterKernelSection();
		s_consoleMutex->Lock();
		for (SIZE_T i = 0; i < size; __Impl_OutputCharacter(message[i++]));
		if (_swapBuffers)
			__Impl_swapBuffers();
		s_consoleMutex->Unlock();
		LeaveKernelSection();
	}
	void ConsoleOutputString(CSTRING message, bool _swapBuffers)
	{
		if (!s_consoleMutex)
			return;
		EnterKernelSection();
		s_consoleMutex->Lock();
		for (SIZE_T i = 0; message[i]; __Impl_OutputCharacter(message[i++]));
		if(_swapBuffers)
			__Impl_swapBuffers();
		s_consoleMutex->Unlock();
		LeaveKernelSection();
	}
	void ConsoleFillLine(char ch, bool _swapBuffers)
	{
		if (!s_consoleMutex)
			return;
		EnterKernelSection();
		s_consoleMutex->Lock();

		if (ch == '\n')
			return;

		for (SIZE_T i = 0; i < s_nCharsHorizontal; i++)
			__Impl_OutputCharacter(ch);

		if (_swapBuffers)
			__Impl_swapBuffers();

		s_consoleMutex->Unlock();
		LeaveKernelSection();
	}
	void ConsoleOutputCharacter(CHAR ch, bool _swapBuffers)
	{
		if (!s_consoleMutex)
			return;
		EnterKernelSection();
		s_consoleMutex->Lock();
		__Impl_OutputCharacter(ch);
		if(_swapBuffers)
			__Impl_swapBuffers();
		s_consoleMutex->Unlock();
		LeaveKernelSection();
	}
	void SetTerminalCursorPosition(DWORD x, DWORD y)
	{
		//UINT16_T pos = y * s_framebufferWidth + x;

		/*outb(0x3D4, 0x0F);
		outb(0x3D5, (UINT8_T)(pos & 0xFF));
		outb(0x3D4, 0x0E);
		outb(0x3D5, (UINT8_T)((pos >> 8) & 0xFF));*/

		s_terminalColumn = x;
		s_terminalRow = y;
	}

	static void __Impl_swapBuffers()
	{
		utils::memcpy(s_framebuffer, s_backbuffer, s_framebufferWidth * s_framebufferHeight);
	}
	// Swap the backbuffer and the viewport.
	void swapBuffers()
	{
		EnterKernelSection();
		s_consoleMutex->Lock();
		__Impl_swapBuffers();
		s_consoleMutex->Unlock();
		LeaveKernelSection();
		//utils::memzero(s_backbuffer, s_framebufferWidth * s_framebufferHeight);
	}
}
