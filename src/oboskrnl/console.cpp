/*
	oboskrnl/console.cpp

	Copyright (c) 2023 Omar Berrow
*/

#ifdef __INTELLISENSE__
#undef __STDC_HOSTED__
#endif
#include <stdint.h>
#include <stddef.h>
#include <limine.h>

#include <console.h>

#include <memory_manipulation.h>

bool strcmp(const char* str1, const char* str2)
{
	if (obos::utils::strlen(str1) != obos::utils::strlen(str2))
		return false;
	for (size_t i = 0; str1[i]; i++)
		if (str1[i] != str2[i])
			return false;
	return true;
}

namespace obos
{
	static volatile struct limine_framebuffer_request framebuffer_request = {
		.id = LIMINE_FRAMEBUFFER_REQUEST,
		.revision = 0
	};
	static volatile struct limine_module_request font_module_request = {
		.id = LIMINE_MODULE_REQUEST,
		.revision = 0,
	};
	uint32_t g_terminalX = 0;
	uint32_t g_terminalY = 0;
	uint32_t g_nCharsHorizontal = 0;
	uint32_t g_nCharsVertical = 0;
	uint32_t g_framebufferWidth = 0;
	uint32_t g_framebufferHeight = 0;
	uint32_t* g_framebuffer = nullptr;
	uint32_t g_foregroundColour = 0;
	uint8_t* g_font = nullptr;
	void EarlyKPanic();

	void plotPixel(uint32_t color, uint32_t x, uint32_t y)
	{
		g_framebuffer[y * g_framebufferWidth + x] = color;
	}

	void putChar(char ch, uint32_t x, uint32_t y, uint32_t fgcolor, uint32_t bgcolor)
	{
		if (!framebuffer_request.response->framebuffers)
			EarlyKPanic();
		if (!g_framebuffer)
		{
			g_framebuffer = (uint32_t*)framebuffer_request.response->framebuffers[0]->address;
			g_framebufferWidth = framebuffer_request.response->framebuffers[0]->width;
			g_framebufferHeight = framebuffer_request.response->framebuffers[0]->height;
			g_nCharsHorizontal = g_framebufferWidth / 16;
			g_nCharsVertical = g_framebufferHeight / 8;
		}
		if (!g_font)
		{
			for (size_t i = 0; i < font_module_request.response->module_count; i++)
			{
				if (strcmp(font_module_request.response->modules[i]->path, "/obos/font.bin"))
				{
					g_font = (uint8_t*)font_module_request.response->modules[i]->address;
					break;
				}
			}
		}
		int cx, cy;
		int mask[8] = { 128,64,32,16,8,4,2,1 };
		const uint8_t* glyph = g_font + (int)ch * 16;
		if (x > g_nCharsHorizontal)
			x = 0;
		if (y > g_framebufferHeight)
			y = 0;
		x <<= 3;
		y += 16;

		for (cy = 0; cy < 16; cy++) {
			for (cx = 0; cx < 8; cx++) {
				plotPixel(glyph[cy] & mask[cx] ? fgcolor : bgcolor, x + cx, y + cy - 12);
			}
		}
	}

	void ConsoleOutput(char ch, uint32_t& x, uint32_t& y)
	{
		switch (ch)
		{
		case '\n':
			y++;
			x = 0;
			break;
		case '\r':
			x = 0;
			break;
		case '\t':
			x += 4 - (x % 4);
			break;
		case '\b':
			x--;
			break;
		default:
			putChar(ch, x++,y, 0xffffffff, 0);
			break;
		}
	}

	void ConsoleOutput(const char* string)
	{
		for(size_t i = 0; string[i]; ConsoleOutput(string[i++]));
	}
	void ConsoleOutput(const char* string, size_t size)
	{
		for (size_t i = 0; i < size; ConsoleOutput(string[i++]));
	}
	void ConsoleOutput(char ch)
	{
		ConsoleOutput(ch, g_terminalX, g_terminalY);
	}

}