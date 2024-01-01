/*
	arch/x86_64/syscall/console_syscalls.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

namespace obos
{
	namespace syscalls
	{
		/// <summary>
		/// Allocates a console for the calling process. If there is already a console, it is reset to white on black colours, at 0,0.
		/// </summary>
		void SyscallAllocConsole();

		/// <summary>
		/// Prints a single character.
		/// </summary>
		/// <param name="ch">The character to print.</param>
		void SyscallConsoleOutputCharacter(void* pars);
		/// <summary>
		/// Prints a character at x,y.
		/// </summary>
		/// <param name="ch">The character to print.</param>
		/// <param name="x">The x position of where to print. This must be within the console bounds.</param>
		/// <param name="y">The y position of where to print. This must be within the console bounds.</param>
		void SyscallConsoleOutputCharacterAt(void* pars);
		/// <summary>
		/// Prints a character at x,y with the colours foregroundColour:backgroundColour.
		/// </summary>
		/// <param name="ch">The character to print.</param>
		/// <param name="foregroundColour">The foreground colour.</param>
		/// <param name="foregroundColour">The background colour.</param>
		/// <param name="x">The x position of where to print. This must be within the console bounds.</param>
		/// <param name="y">The y position of where to print. This must be within the console bounds.</param>
		void SyscallConsoleOutputCharacterAtWithColour(void* pars);
		/// <summary>
		/// Prints a string.
		/// </summary>
		/// <param name="string">The string to print.</param>
		void SyscallConsoleOutputString(void* pars);
		/// <summary>
		/// Prints a string.
		/// </summary>
		/// <param name="string">The string to print.</param>
		/// <param name="size">The amount of characters to print.</param>
		void SyscallConsoleOutputStringSz(void* pars);
		/// <summary>
		/// Sets the position of the cursor.
		/// </summary>
		/// <param name="x">The new x position.</param>
		/// <param name="y">The new y position.</param>
		void SyscallConsoleSetPosition(uint32_t* pars);
		/// <summary>
		/// Retrieves the position of the cursor.
		/// </summary>
		/// <param name="x">A pointer to where the x position should be stored.</param>
		/// <param name="y">A pointer to where the y position should be stored.</param>
		void SyscallConsoleGetPosition(uint32_t** pars);
		/// <summary>
		/// Sets the console's colour.
		/// </summary>
		/// <param name="foregroundColour">The new foreground colour.</param>
		/// <param name="backgroundColour">The new background colour.</param>
		/// <param name="clearConsole">Whether to overwrite all text with "backgroundColour"</param>
		void SyscallConsoleSetColour(uint32_t* pars);
		/// <summary>
		/// Retrieves the console's colour.
		/// </summary>
		/// <param name="foregroundColour">A pointer to where the foreground colour should be stored.</param>
		/// <param name="backgroundColour">A pointer to where the background colour should be stored.</param>
		void SyscallConsoleGetColour(uint32_t** pars);
		/// <summary>
		/// Sets the console's font. This must be a 8x16 bitmap font. This must also last the entire lifetime of the console.
		/// </summary>
		/// <param name="font">The new font.</param>
		void SyscallConsoleSetFont(uint8_t** font);
		/// <summary>
		/// Retrieves the font.
		/// </summary>
		/// <param name="font">A pointer to where the font should be stored. This is allocated with VirtualAlloc and must be freed to avoid memory leaks.</param>
		void SyscallConsoleGetFont(uint8_t*** font);
		/// <summary>
		/// Sets the framebuffer to draw to.
		/// </summary>
		/// <param name="framebuffer">The new framebuffer.</param>
		void SyscallConsoleSetFramebuffer(void* pars);
		/// <summary>
		/// Retrieves the framebuffer that's being drawn to.
		/// </summary>
		/// <param name="framebuffer">A pointer to where the framebuffer should be stored.</param>
		void SyscallConsoleGetFramebuffer(void* pars);
		/// <summary>
		/// Retrieves the console bounds.
		/// </summary>
		/// <param name="horizontal">A pointer to where the horizontal bounds should be stored.</param>
		/// <param name="vertical">A pointer to where the vertical bounds should be stored.</param>
		void SyscallConsoleGetConsoleBounds(uint32_t** pars);

		/// <summary>
		/// Frees the console previously allocated.
		/// </summary>
		void SyscallFreeConsole();
	}
}