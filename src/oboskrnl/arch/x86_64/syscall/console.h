/*
	arch/x86_64/syscall/console.h

	Copyright (c) 2023-2024 Omar Berrow
*/

#pragma once

#include <int.h>

#include <arch/x86_64/syscall/handle.h>

namespace obos
{
	namespace syscalls
	{
		uintptr_t ConsoleSyscallHandler(uint64_t syscall, void* args);

		/// <summary>
		/// Syscall Number: 25<para></para>
		/// Initializes the current process' console. This can only be called once.<para></para>
		/// By default, if this function isn't called, any console syscalls will use the kernel console.
		/// </summary>
		/// <returns>Whether the function succeeded (true), or not (false).</returns>
		bool SyscallInitializeProcessConsole();

		/// <summary>
		/// Syscall Number: 26<para></para>
		/// Prints a string on the calling process' console.
		/// </summary>
		/// <param name="string">The string to print.</param>
		/// <param name="size">The amount of characters to print.</param>
		void SyscallConsoleOutput(const char* string, size_t size);
		/// <summary>
		/// Syscall Number: 27<para></para>
		/// Prints a single character on the calling process' console.
		/// </summary>
		/// <param name="ch">The character to print.</param>
		void SyscallConsoleOutput(char ch);
		/// <summary>
		/// Syscall Number: 28<para></para>
		/// Prints a character at x,y on the calling process' console.
		/// </summary>
		/// <param name="ch">The character to print.</param>
		/// <param name="x">The x position of where to print. This must be within the console bounds.</param>
		/// <param name="y">The y position of where to print. This must be within the console bounds.</param>
		void SyscallConsoleOutput(char ch, uint32_t x, uint32_t y);
		/// <summary>
		/// Syscall Number: 29<para></para>
		/// Prints a character at x,y with the colours foregroundColour:backgroundColour on the calling process' console.
		/// </summary>
		/// <param name="ch">The character to print.</param>
		/// <param name="foregroundColour">The foreground colour.</param>
		/// <param name="foregroundColour">The background colour.</param>
		/// <param name="x">The x position of where to print. This must be within the console bounds.</param>
		/// <param name="y">The y position of where to print. This must be within the console bounds.</param>
		void SyscallConsoleOutput(char ch, uint32_t foregroundColour, uint32_t backgroundColour, uint32_t x, uint32_t y);
		
		/// <summary>
		/// Syscall Number: 30<para></para>
		/// Sets the position of the cursor for the calling process' console.
		/// </summary>
		/// <param name="x">The new x position.</param>
		/// <param name="y">The new y position.</param>
		void SyscallSetPosition(uint32_t x, uint32_t y);
		/// <summary>
		/// Syscall Number: 31<para></para>
		/// Retrieves the position of the cursor for the calling process' console.
		/// </summary>
		/// <param name="x">A pointer to where the x position should be stored.</param>
		/// <param name="y">A pointer to where the y position should be stored.</param>
		void SyscallGetPosition(uint32_t* x, uint32_t* y);
		/// <summary>
		/// Syscall Number: 32<para></para>
		/// Sets the colour of the calling process' console.
		/// </summary>
		/// <param name="foregroundColour">The new foreground colour.</param>
		/// <param name="backgroundColour">The new background colour.</param>
		/// <param name="clearConsole">Whether to overwrite all text with "backgroundColour"</param>
		void SyscallSetColour(uint32_t foregroundColour, uint32_t backgroundColour, bool clearConsole);
		/// <summary>
		/// Syscall Number: 33<para></para>
		/// Retrieves the colour of the calling process' console.
		/// </summary>
		/// <param name="foregroundColour">A pointer to where the foreground colour should be stored.</param>
		/// <param name="backgroundColour">A pointer to where the background colour should be stored.</param>
		void SyscallGetColour(uint32_t* foregroundColour, uint32_t* backgroundColour);
		/// <summary>
		/// Syscall Number: 34<para></para>
		/// Sets the console's font of the calling process' console. This must be a 8x16 bitmap font.<para></para>
		/// If SyscallInitializeProcessConsole was never called, this function call is ignored.
		/// </summary>
		/// <param name="font">The new font.</param>
		void SyscallSetFont(uint8_t* font);
		/// <summary>
		/// Syscall Number: 35<para></para>
		/// Retrieves the font for the calling process' console.
		/// </summary>
		/// <param name="font">A pointer to where the font should be stored. The font will be allocated with the VMM, so to free just call VirtualFree on the font.</param>
		void SyscallGetFont(uint8_t** font);
		/// <summary>
		/// Syscall Number: 36<para></para>
		/// Retrieves the console bounds for the calling process' console.
		/// </summary>
		/// <param name="horizontal">A pointer to where the horizontal bounds should be stored.</param>
		/// <param name="vertical">A pointer to where the vertical bounds should be stored.</param>
		void SyscallGetConsoleBounds(uint32_t* horizontal, uint32_t* vertical);
		
		/// <summary>
		/// Syscall Number: 37<para></para>
		/// Swaps the buffers for the calling process' console.
		/// </summary>
		void SyscallSwapBuffers();
		/// <summary>
		/// Syscall Number: 38<para></para>
		/// Sets the buffer to draw to.<para></para>
		/// If SyscallInitializeProcessConsole was never called, this function call is ignored.
		/// </summary>
		/// <param name="isBackbuffer">Whether to set the drawing buffer to the back buffer (true) or to the raw framebuffer (false).</param>
		void SyscallSetDrawBuffer(bool isBackbuffer);
	}
}