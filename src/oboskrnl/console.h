/*
	oboskrnl/console.h

	Copyright (c) 2023 Omar Berrow
*/

#pragma once

#include <stdint.h>
#include <stddef.h>

namespace obos
{ 
	void ConsoleOutput(const char* string);
	void ConsoleOutput(const char* string, size_t size);
	void ConsoleOutput(char ch);
	void ConsoleOutput(char ch, uint32_t& x, uint32_t& y);
}