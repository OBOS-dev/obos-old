/*
	oboskrnl/console.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{ 
	struct con_framebuffer
	{
		uint32_t* addr;
		uint32_t width;
		uint32_t height;
	};

	class Console
	{
	public:
		Console() = default;
		Console(void* font, con_framebuffer output);
		void Initialize(void* font, con_framebuffer output);

		void ConsoleOutput(const char* string);
		void ConsoleOutput(const char* string, size_t size);
		void ConsoleOutput(char ch);
		void ConsoleOutput(char ch, uint32_t& x, uint32_t& y);
		void ConsoleOutput(char ch, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t& x, uint32_t& y);

		void SetPosition(uint32_t x, uint32_t y);
		void GetPosition(uint32_t* x, uint32_t *y);
		void SetColour(uint32_t foregroundColour, uint32_t backgroundColour);
		void GetColour(uint32_t* foregroundColour, uint32_t* backgroundColour);
		void SetFont(uint8_t* font);
		void GetFont(uint8_t** font);
		void SetFramebuffer(con_framebuffer framebuffer);
		void GetFramebuffer(con_framebuffer* framebuffer);
		void GetConsoleBounds(uint32_t* horizontal, uint32_t* vertical);

		// Copies from the framebuffer specified in the parameter to the this->m_framebuffer.
		// This function can be used for back buffering.
		void CopyFrom(con_framebuffer* );
	private:
		void plotPixel(uint32_t color, uint32_t x, uint32_t y);
		void putChar(char ch, uint32_t x, uint32_t y, uint32_t fgcolor, uint32_t bgcolor);
		con_framebuffer m_framebuffer;
		uint32_t m_terminalX = 0;
		uint32_t m_terminalY = 0;
		uint32_t m_nCharsHorizontal = 0;
		uint32_t m_nCharsVertical = 0;
		uint32_t m_foregroundColour = 0;
		uint32_t m_backgroundColour = 0;
		uint8_t* m_font = nullptr;
	};
}