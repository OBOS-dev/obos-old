/*
	oboskrnl/console.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>
#include <export.h>

#include <multitasking/locks/mutex.h>

namespace obos
{ 
	struct con_framebuffer
	{
		uint32_t* addr = nullptr;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t pitch = 0;
	};

	class Console
	{
	public:
		OBOS_EXPORT Console() = default;
		OBOS_EXPORT Console(void* font, const con_framebuffer& output, bool lockCanUseMultitasking = true);
		OBOS_EXPORT void Initialize(void* font, const con_framebuffer& output, bool lockCanUseMultitasking = true);

		/// <summary>
		/// Prints a string.
		/// </summary>
		/// <param name="string">The string to print.</param>
		OBOS_EXPORT void ConsoleOutput(const char* string);
		/// <summary>
		/// Prints a string.
		/// </summary>
		/// <param name="string">The string to print.</param>
		/// <param name="size">The amount of characters to print.</param>
		OBOS_EXPORT void ConsoleOutput(const char* string, size_t size);
		/// <summary>
		/// Prints a single character.
		/// </summary>
		/// <param name="ch">The character to print.</param>
		OBOS_EXPORT void ConsoleOutput(char ch);
		/// <summary>
		/// Prints a character at x,y.
		/// </summary>
		/// <param name="ch">The character to print.</param>
		/// <param name="x">The x position of where to print. This must be within the console bounds.</param>
		/// <param name="y">The y position of where to print. This must be within the console bounds.</param>
		OBOS_EXPORT void ConsoleOutput(char ch, uint32_t& x, uint32_t& y);
		/// <summary>
		/// Prints a character at x,y with the colours foregroundColour:backgroundColour.
		/// </summary>
		/// <param name="ch">The character to print.</param>
		/// <param name="foregroundColour">The foreground colour.</param>
		/// <param name="foregroundColour">The background colour.</param>
		/// <param name="x">The x position of where to print. This must be within the console bounds.</param>
		/// <param name="y">The y position of where to print. This must be within the console bounds.</param>
		OBOS_EXPORT void ConsoleOutput(char ch, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t& x, uint32_t& y);

		/// <summary>
		/// Sets the position of the cursor.
		/// </summary>
		/// <param name="x">The new x position.</param>
		/// <param name="y">The new y position.</param>
		OBOS_EXPORT void SetPosition(uint32_t x, uint32_t y);
		/// <summary>
		/// Retrieves the position of the cursor.
		/// </summary>
		/// <param name="x">A pointer to where the x position should be stored.</param>
		/// <param name="y">A pointer to where the y position should be stored.</param>
		OBOS_EXPORT void GetPosition(uint32_t* x, uint32_t *y);
		/// <summary>
		/// Sets the console's colour.
		/// </summary>
		/// <param name="foregroundColour">The new foreground colour.</param>
		/// <param name="backgroundColour">The new background colour.</param>
		/// <param name="clearConsole">Whether to overwrite all text with "backgroundColour"</param>
		OBOS_EXPORT void SetColour(uint32_t foregroundColour, uint32_t backgroundColour, bool clearConsole = false);
		/// <summary>
		/// Retrieves the console's colour.
		/// </summary>
		/// <param name="foregroundColour">A pointer to where the foreground colour should be stored.</param>
		/// <param name="backgroundColour">A pointer to where the background colour should be stored.</param>
		OBOS_EXPORT void GetColour(uint32_t* foregroundColour, uint32_t* backgroundColour) const;
		/// <summary>
		/// Sets the console's font. This must be a 8x16 bitmap font.
		/// </summary>
		/// <param name="font">The new font.</param>
		OBOS_EXPORT void SetFont(uint8_t* font);
		/// <summary>
		/// Retrieves the font.
		/// </summary>
		/// <param name="font">A pointer to where the font should be stored.</param>
		OBOS_EXPORT void GetFont(uint8_t** font);
		/// <summary>
		/// Sets the framebuffer to draw to.
		/// </summary>
		/// <param name="framebuffer">The new framebuffer.</param>
		OBOS_EXPORT void SetFramebuffer(con_framebuffer framebuffer);
		/// <summary>
		/// Retrieves the framebuffer that's being drawn to.
		/// </summary>
		/// <param name="framebuffer">A pointer to where the framebuffer should be stored.</param>
		OBOS_EXPORT void GetFramebuffer(con_framebuffer* framebuffer) const;
		/// <summary>
		/// Retrieves the console bounds.
		/// </summary>
		/// <param name="horizontal">A pointer to where the horizontal bounds should be stored.</param>
		/// <param name="vertical">A pointer to where the vertical bounds should be stored.</param>
		OBOS_EXPORT void SetBackBuffer(const con_framebuffer& buffer);
		OBOS_EXPORT void GetBackBuffer(con_framebuffer* buffer) const;
		OBOS_EXPORT void GetConsoleBounds(uint32_t* horizontal, uint32_t* vertical) const;

		OBOS_EXPORT void Unlock() { m_lock.Unlock(); };

		OBOS_EXPORT void SwapBuffers();
		OBOS_EXPORT void SetDrawBuffer(bool isBackbuffer);
	private:
		friend void arch_kmain();
		void putChar(char ch, uint32_t x, uint32_t y, uint32_t fgcolor, uint32_t bgcolor);
		void newlineHandler(uint32_t& x, uint32_t& y);
		void __ImplConsoleOutputChar(char ch, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t& x, uint32_t& y, bool incrementCallsSinceSwap = true);
		con_framebuffer m_framebuffer{};
		con_framebuffer m_backbuffer{};
		con_framebuffer* m_drawingBuffer = &m_framebuffer;
		uint32_t m_terminalX = 0;
		uint32_t m_terminalY = 0;
		uint32_t m_nCharsHorizontal = 0;
		uint32_t m_nCharsVertical = 0;
		uint32_t m_foregroundColour = 0;
		uint32_t m_backgroundColour = 0;
		uint8_t* m_font = nullptr;
		uint32_t* m_modificationArray = nullptr; // The size of this array should be m_nCharsVertical, or nullptr when it doesn't exist.
		// Only incremented by ConsoleOutput(char ch) when 'ch' is newline. Incremented by ConsoleOutput(const char*) or ConsoleOutput(const char*,size_t)
		size_t m_nCallsSinceLastSwap = 0;
#ifdef OBOS_DEBUG
		constexpr static size_t maxCountsUntilSwap = 1;
#else
		constexpr static size_t maxCountsUntilSwap = 5;
#endif
		locks::Mutex m_lock;
	};
	extern OBOS_EXPORT Console g_kernelConsole;
}