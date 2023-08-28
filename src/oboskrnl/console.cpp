/*
	console.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <console.h>

#include <boot/multiboot.h>
#include <inline-asm.h>

#include <utils/memory.h>
#include <utils/bitfields.h>

#include <memory_manager/paging/allocate.h>

#include <multitasking/mutex/mutexHandle.h>

#include <new>

#include <boot/boot.h>

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
	PBYTE font = nullptr;
	static multitasking::MutexHandle* s_consoleMutex;
	static utils::IntegerBitfield s_modifiedLines[2];
	static bool lineSequence = false;

	// Internal functions.
	
	/*static void memset(void* block, char val, SIZE_T size)
	{
		for (SIZE_T i = 0; i < size; ((PBYTE)block)[i++] = val);
	}*/
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
		if (x > s_nCharsHorizontal)
			x = 0;
		if (y > s_framebufferHeight)
			y = 0;
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
		if (++s_terminalRow >= s_nCharsVertical - 1)
		{
			s_reachedEndTerminal = true;
			// Erase the first line of characters.
			utils::memzero(s_backbuffer, s_framebufferWidth * 16 * 4);
			utils::memcpy(s_backbuffer, s_backbuffer + s_framebufferWidth * 16, (s_framebufferWidth * s_framebufferHeight * 4) - s_framebufferWidth * 16 * 4);
			utils::dwMemset((DWORD*)(GET_FUNC_ADDR(s_backbuffer) + (s_framebufferWidth * s_framebufferHeight * 4) - s_framebufferWidth * 16 * 4),
				s_backgroundColor, s_framebufferWidth * 16);
			for (int i = 0; i < 2; i++)
				s_modifiedLines[i].setBitfield(0);
			utils::memcpy(s_framebuffer, s_backbuffer, s_framebufferWidth * s_framebufferHeight * 4);
		}
		if (s_reachedEndTerminal)
			s_terminalRow = s_nCharsVertical - 2;
		if (lineSequence)
			s_terminalColumn = 0;
	}

	// API

	void InitializeConsole(UINT32_T foregroundColor, UINT32_T backgroundColor)
	{
		EnterKernelSection();
		lineSequence = !utils::strcmp(CONSOLE_NEWLINE_SEQUENCE, "LF");
		s_backbuffer = nullptr;
		new (s_modifiedLines) utils::IntegerBitfield{};
		new (s_modifiedLines + 1) utils::IntegerBitfield{};
		s_framebuffer = (UINT32_T*)0xFFCFF000;
		s_terminalColumn = 0;
		s_terminalRow = 0;
		s_framebufferHeight = g_multibootInfo->framebuffer_height;
		s_framebufferWidth = g_multibootInfo->framebuffer_width;
		s_nCharsVertical = s_framebufferHeight / 16;
		s_nCharsHorizontal = s_framebufferWidth / 8;
		s_consoleMutex = new multitasking::MutexHandle{};
		s_backbuffer = (UINT32_T*)memory::VirtualAlloc(nullptr, (s_framebufferWidth * s_framebufferHeight * sizeof(DWORD)) >> 12, memory::VirtualAllocFlags::WRITE_ENABLED);
		s_consoleMutex->Lock();
#ifndef __x86_64__
		utils::dwMemset(s_backbuffer, backgroundColor, s_framebufferWidth * s_framebufferHeight * 4);
#endif
		font = (PBYTE)(((multiboot_module_t*)g_multibootInfo->mods_addr)->mod_start);
		SetConsoleColor(foregroundColor, backgroundColor);
		utils::dwMemset(s_framebuffer, s_backgroundColor, s_framebufferWidth * s_framebufferHeight);
		s_consoleMutex->Unlock();
		UpdateCursorPosition();
		LeaveKernelSection();
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
			for (DWORD i = 0; i < x && s_terminalColumn < s_framebufferWidth; i++)
				putcharAt(++s_terminalColumn, s_terminalRow * 16, ' ', s_foregroundColor, s_backgroundColor);
			s_modifiedLines[s_terminalRow >> 5].setBit(s_terminalRow % 32);
			break;
		}
		case '\b':
		{
			if (s_terminalColumn == 0)
				// TODO: Make a beep here.
				break;
			putcharAt(--s_terminalColumn, s_terminalRow * 16, ' ', s_foregroundColor, s_backgroundColor);
			s_modifiedLines[s_terminalRow >> 5].setBit(s_terminalRow % 32);
			return;
		}
		case '\0':
			break;
		case 15:
			s_terminalColumn++;
			break;
		default:
		{
			putcharAt(s_terminalColumn, s_terminalRow * 16, ch, s_foregroundColor, s_backgroundColor);
			if (++s_terminalColumn >= s_nCharsHorizontal)
			{
				s_terminalColumn = 0;
				on_newline();
			}
			s_modifiedLines[s_terminalRow >> 5].setBit(s_terminalRow % 32);
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
		LeaveKernelSection();
		if (_swapBuffers)
			__Impl_swapBuffers();
		s_consoleMutex->Unlock();
	}
	void ConsoleOutputString(CSTRING message, bool _swapBuffers)
	{
		if (!s_consoleMutex)
			return;
		EnterKernelSection();
		s_consoleMutex->Lock();
		for (SIZE_T i = 0; message[i]; __Impl_OutputCharacter(message[i++]));
		LeaveKernelSection();
		if(_swapBuffers)
			__Impl_swapBuffers();
		s_consoleMutex->Unlock();
	}
	void ConsoleFillLine(char ch, bool _swapBuffers)
	{
		if (!s_consoleMutex)
			return;
		EnterKernelSection();
		s_consoleMutex->Lock();

		if (ch == '\n')
			return;

		for (SIZE_T i = s_terminalColumn; i < s_nCharsHorizontal; i++)
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
	void ConsoleOutputCharacter(CHAR ch, DWORD x, DWORD y, bool _swapBuffers)
	{
		if (!s_consoleMutex)
			return;
		EnterKernelSection();
		s_consoleMutex->Lock();
		DWORD oldX = s_terminalColumn;
		DWORD oldY = s_terminalRow;
		s_terminalColumn = x;
		s_terminalRow = y;
		__Impl_OutputCharacter(ch);
		if (_swapBuffers)
			__Impl_swapBuffers();
		s_terminalColumn = oldX;
		s_terminalRow = oldY;
		s_consoleMutex->Unlock();
		LeaveKernelSection();
	}
	void SetTerminalCursorPosition(DWORD x, DWORD y)
	{
		s_terminalColumn = x;
		s_terminalRow = y;
	}

	void GetTerminalCursorPosition(DWORD& x, DWORD& y)
	{
		x = s_terminalColumn;
		y = s_terminalRow;
	}

	static void __Impl_swapBuffers()
	{
		for (int b = 0; b < 32; b++)
		{
			if (s_modifiedLines[0].getBit(b))
			{
				int scanline = b * 16 + 6;
				s_modifiedLines[0].clearBit(b);
				utils::dwMemcpy(s_framebuffer + (scanline * s_framebufferWidth), s_backbuffer + (scanline * s_framebufferWidth), s_framebufferWidth * 16);
			}
		}
		for (int b = 0; b < 16; b++)
		{
			if (s_modifiedLines[1].getBit(b))
			{
				int scanline = ((32 + b) * 16) + 6;
				s_modifiedLines[1].clearBit(b);
				utils::dwMemcpy(s_framebuffer + (scanline * s_framebufferWidth), s_backbuffer + (scanline * s_framebufferWidth), (s_framebufferWidth * 16) - 6);
			}
		}
	}
	void ClearConsole()
	{
		EnterKernelSection();
		s_consoleMutex->Lock();
		utils::dwMemset(s_backbuffer, 0, s_framebufferWidth * s_framebufferHeight);
		utils::dwMemset(s_framebuffer, 0, s_framebufferWidth * s_framebufferHeight);
		s_consoleMutex->Unlock();
		LeaveKernelSection();
	}
	// Swap the backbuffer with the viewport.
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
