/*
	oboskrnl/console.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <int.h>

#include <multitasking/locks/mutex.h>

namespace obos
{ 
	struct con_framebuffer
	{
		uint32_t* addr;
		uint32_t width;
		uint32_t height;
		uint32_t pitch;
	};

	class Console
	{
	public:
		Console() = default;
		Console(void* font, con_framebuffer output, bool lockCanUseMultitasking = true);
		void Initialize(void* font, con_framebuffer output, bool lockCanUseMultitasking = true);

		/// <summary>
		/// Prints a string.
		/// </summary>
		/// <param name="string">The string to print.</param>
		void ConsoleOutput(const char* string);
		/// <summary>
		/// Prints a string.
		/// </summary>
		/// <param name="string">The string to print.</param>
		/// <param name="size">The amount of characters to print.</param>
		void ConsoleOutput(const char* string, size_t size);
		/// <summary>
		/// Prints a single character.
		/// </summary>
		/// <param name="ch">The character to print.</param>
		void ConsoleOutput(char ch);
		/// <summary>
		/// Prints a character at x,y.
		/// </summary>
		/// <param name="ch">The character to print.</param>
		/// <param name="x">The x position of where to print. This must be within the console bounds.</param>
		/// <param name="y">The y position of where to print. This must be within the console bounds.</param>
		void ConsoleOutput(char ch, uint32_t& x, uint32_t& y);
		/// <summary>
		/// Prints a character at x,y with the colours foregroundColour:backgroundColour.
		/// </summary>
		/// <param name="ch">The character to print.</param>
		/// <param name="foregroundColour">The foreground colour.</param>
		/// <param name="foregroundColour">The background colour.</param>
		/// <param name="x">The x position of where to print. This must be within the console bounds.</param>
		/// <param name="y">The y position of where to print. This must be within the console bounds.</param>
		void ConsoleOutput(char ch, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t& x, uint32_t& y);

		/// <summary>
		/// Sets the position of the cursor.
		/// </summary>
		/// <param name="x">The new x position.</param>
		/// <param name="y">The new y position.</param>
		void SetPosition(uint32_t x, uint32_t y);
		/// <summary>
		/// Retrieves the position of the cursor.
		/// </summary>
		/// <param name="x">A pointer to where the x position should be stored.</param>
		/// <param name="y">A pointer to where the y position should be stored.</param>
		void GetPosition(uint32_t* x, uint32_t *y);
		/// <summary>
		/// Sets the console's colour.
		/// </summary>
		/// <param name="foregroundColour">The new foreground colour.</param>
		/// <param name="backgroundColour">The new background colour.</param>
		/// <param name="clearConsole">Whether to overwrite all text with "backgroundColour"</param>
		void SetColour(uint32_t foregroundColour, uint32_t backgroundColour, bool clearConsole = false);
		/// <summary>
		/// Retrieves the console's colour.
		/// </summary>
		/// <param name="foregroundColour">A pointer to where the foreground colour should be stored.</param>
		/// <param name="backgroundColour">A pointer to where the background colour should be stored.</param>
		void GetColour(uint32_t* foregroundColour, uint32_t* backgroundColour);
		/// <summary>
		/// Sets the console's font. This must be a 8x16 bitmap font.
		/// </summary>
		/// <param name="font">The new font.</param>
		void SetFont(uint8_t* font);
		/// <summary>
		/// Retrieves the font.
		/// </summary>
		/// <param name="font">A pointer to where the font should be stored.</param>
		void GetFont(uint8_t** font);
		/// <summary>
		/// Sets the framebuffer to draw to.
		/// </summary>
		/// <param name="framebuffer">The new framebuffer.</param>
		void SetFramebuffer(con_framebuffer framebuffer);
		/// <summary>
		/// Retrieves the framebuffer that's being drawn to.
		/// </summary>
		/// <param name="framebuffer">A pointer to where the framebuffer should be stored.</param>
		void GetFramebuffer(con_framebuffer* framebuffer);
		/// <summary>
		/// Retrieves the console bounds.
		/// </summary>
		/// <param name="horizontal">A pointer to where the horizontal bounds should be stored.</param>
		/// <param name="vertical">A pointer to where the vertical bounds should be stored.</param>
		void GetConsoleBounds(uint32_t* horizontal, uint32_t* vertical);

		void Unlock() { m_lock.Unlock(); };

		// Copies from the framebuffer specified in the parameter to the this->m_framebuffer.
		// This function can be used for back buffering.
		void CopyFrom(con_framebuffer* buffer);
	private:
		void putChar(char ch, uint32_t x, uint32_t y, uint32_t fgcolor, uint32_t bgcolor);
		void newlineHandler(uint32_t& x, uint32_t& y);
		void __ImplConsoleOutputChar(char ch, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t& x, uint32_t& y);
		con_framebuffer m_framebuffer;
		uint32_t m_terminalX = 0;
		uint32_t m_terminalY = 0;
		uint32_t m_nCharsHorizontal = 0;
		uint32_t m_nCharsVertical = 0;
		uint32_t m_foregroundColour = 0;
		uint32_t m_backgroundColour = 0;
		uint8_t* m_font = nullptr;
		locks::Mutex m_lock;
	};
}