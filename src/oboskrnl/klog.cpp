/*
	klog.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <boot/multiboot.h>

#include <types.h>
#include <klog.h>
#include <console.h>
#include <inline-asm.h>

#include <utils/memory.h>

#include <memory_manager/paging/allocate.h>

#include <descriptors/idt/idt.h>
#include <descriptors/idt/pic.h>

#include <external/amalgamated-dist/Zydis.h>

#include <boot/boot.h>

// Credit: http://www.strudel.org.uk/itoa/
static char* itoa(int value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, * ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
	} while (value);

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}
void strreverse(char* begin, int size)
{
	int i = 0;
	char tmp = 0;

	for (i = 0; i < size / 2; i++)
	{
		tmp = begin[i];
		begin[i] = begin[size - i - 1];
		begin[size - i - 1] = tmp;
	}
}
// Credit: http://www.strudel.org.uk/itoa/
char* itoa_unsigned(unsigned int value, char* result, int base)
{
	// check that the base if valid

	if (base < 2 || base > 16)
	{
		*result = 0;
		return result;
	}

	char* out = result;

	unsigned int quotient = value;

	do
	{

		unsigned int abs = /*(quotient % base) < 0 ? (-(quotient % base)) : */(quotient % base);

		*out = "0123456789abcdef"[abs];

		++out;

		quotient /= base;

	} while (quotient);

	strreverse(result, out - result);

	return result;

}

static char toupper(char ch)
{
	char tmp = ch - 'a';
	if (tmp > 26 || tmp < 0)
		return ch;
	return tmp + 'A';
}
// This takes half a page of memory...
//static constexpr const char skull_ascii_art[] =
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
//"                 uuuuuuu\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "             uu$$$$$$$$$$$uu\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "          uu$$$$$$$$$$$$$$$$$uu\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "         u$$$$$$$$$$$$$$$$$$$$$u\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "        u$$$$$$$$$$$$$$$$$$$$$$$u\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "       u$$$$$$$$$$$$$$$$$$$$$$$$$u\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "       u$$$$$$$$$$$$$$$$$$$$$$$$$u\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "       u$$$$$$\"   \"$$$\"   \"$$$$$$u\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "       \"$$$$\"      u$u       $$$$\"\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "        $$$u       u$u       u$$$\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "        $$$u      u$$$u      u$$$\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "         \"$$$$uu$$$   $$$uu$$$$\"\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "          \"$$$$$$$\"   \"$$$$$$$\"\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "            u$$$$$$$u$$$$$$$u\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "             u$\"$\"$\"$\"$\"$\"$u\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "  uuu        $$u$ $ $ $ $u$$       uuu\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// " u$$$$        $$$$$u$u$u$$$       u$$$$\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "  $$$$$uu      \"$$$$$$$$$\"     uu$$$$$$\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "u$$$$$$$$$$$uu    \"\"\"\"\"    uuuu$$$$$$$$$$\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "$$$$\"\"\"$$$$$$$$$$uuu   uu$$$$$$$$$\"\"\"$$$\"\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// " \"\"\"      \"\"$$$$$$$$$$$uu \"\"$\"\"\"\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "           uuuu \"\"$$$$$$$$$$uuu\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "  u$$$uuu$$$$$$$$$uu \"\"$$$$$$$$$$$uuu$$$\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "  $$$$$$$$$$\"\"\"\"           \"\"$$$$$$$$$$$\"\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "   \"$$$$$\"                      \"\"$$$$\"\"\r\n"
//"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
// "     $$$\"                         $$$$\"\r\n"
//
//;

extern "C" PVOID getEBP();

namespace obos
{
	void consoleOutputCharacter(BYTE ch, PVOID)
	{
		obos::ConsoleOutputCharacter(ch, false);
		outb(0x3f8, ch);
		outb(0xe9, ch);
	}
	void _vprintf(void(*printChar)(BYTE ch, PVOID userdata), volatile PVOID printCharUserdata, CSTRING format, va_list list)
	{
		CSTRING _format = format;
		for (; *_format; _format++)
		{
			char ch = *_format;
			if (ch == '%')
			{
				ch = *++_format;
				switch (ch)
				{
				case 'd':
				case 'i':
				{
					char str[12];
					utils::memzero(str, 12);
					itoa(va_arg(list, int), str, 10);
					for (int i = 0; str[i]; i++)
						printChar(str[i], printCharUserdata);
					break;
				}
				case 'u':
				{
					char str[12];
					utils::memzero(str, 12);
					itoa_unsigned(va_arg(list, int), str, 10);
					for (int i = 0; str[i]; i++)
						printChar(str[i], printCharUserdata);
					break;
				}
				case 'o':
				{
					char str[13];
					utils::memzero(str, 13);
					itoa_unsigned(va_arg(list, int), str, 8);
					for (int i = 0; str[i]; i++)
						printChar(str[i], printCharUserdata);
					break;
				}
				case 'x':
				{
					char str[9];
					utils::memzero(str, 9);
					itoa_unsigned(va_arg(list, unsigned int), str, 16);
					for (int i = 0; str[i]; i++)
						printChar(str[i], printCharUserdata);
					break;
				}
				case 'X':
				{
					char str[9];
					utils::memzero(str, 9);
					itoa_unsigned(va_arg(list, unsigned int), str, 16);

					for (int i = 0; str[i]; i++)
						printChar(toupper(str[i]), printCharUserdata);
					break;
				}
				case 'c':
				{
					printChar(va_arg(list, int), printCharUserdata);
					break;
				}
				case 's':
				{
					char* str = va_arg(list, char*);
					for (int i = 0; str[i]; printChar(str[i++], printCharUserdata));
					break;
				}
				case 'p':
				{
					char str[9];
					utils::memzero(str, 17);
					itoa_unsigned(va_arg(list, UINTPTR_T), str, 16);
					printChar('0', printCharUserdata);
					printChar('x', printCharUserdata);
					for (int i = 0; str[i]; i++)
						printChar(str[i], printCharUserdata);
					break;
				}
				case '%':
				{
					printChar('%', printCharUserdata);
					break;
				}
				case '\0':
					return;
				default:
					break;
				}
				continue;
			}
			printChar(ch, printCharUserdata);

		}
	}

	void printf(CSTRING format, ...)
	{
		va_list list; va_start(list, format);
		_vprintf(consoleOutputCharacter, nullptr, format, list);
		va_end(list);
		swapBuffers();
	}
	static void sprintf_callback(BYTE ch, PVOID userData)
	{
		STRING output = *reinterpret_cast<STRING*>(userData);
		SIZE_T& _index = **(reinterpret_cast<SIZE_T**>(userData) + 1);
		if (output)
			output[_index++] = ch;
		else
			_index++;
	}
	SIZE_T sprintf(STRING output, CSTRING format, ...)
	{
		va_list list; va_start(list, format);
		SIZE_T index = 0;
		UINTPTR_T udata[2] = { GET_FUNC_ADDR(output), GET_FUNC_ADDR(&index) };
		_vprintf(sprintf_callback, udata, format, list);
		va_end(list);
		return index;
	}
	void vprintf(CSTRING format, va_list list)
	{
		_vprintf(consoleOutputCharacter, nullptr, format, list);
		swapBuffers();
	}

	static void irq1(const interrupt_frame*)
	{
		extern void RestartComputer();
		RestartComputer();
	}
	void kpanic(PVOID printStackTracePar, PVOID eip, CSTRING format, ...)
	{
		asm volatile("cli");
		EnterKernelSection();

		extern SIZE_T s_framebufferWidth;
		extern SIZE_T s_framebufferHeight;
		extern DWORD s_terminalRow;
		extern DWORD s_terminalColumn;
		extern UINT32_T* s_framebuffer;
		extern bool s_reachedEndTerminal;
		extern UINT32_T* s_backbuffer;
		if (s_framebuffer && s_backbuffer)
		{
			constexpr DWORD kpanicBackgroundColour = 0xFFCB0035;
			SetConsoleColor(0xFFFFFFFF, kpanicBackgroundColour);

			utils::dwMemset(s_framebuffer, kpanicBackgroundColour, s_framebufferWidth * s_framebufferHeight);
			utils::dwMemset(s_backbuffer, kpanicBackgroundColour, s_framebufferWidth * s_framebufferHeight);
			s_terminalRow = 0;
			s_terminalColumn = 0;
			s_reachedEndTerminal = false;

			va_list list; va_start(list, format);
			_vprintf(consoleOutputCharacter, nullptr, format, list);
			va_end(list);

			printf_noFlush("Stack trace:\r\n");
			printStackTrace(printStackTracePar, "\t");

			if(eip)
				disassemble(eip, "\t");

			DWORD x = s_terminalColumn;
			DWORD y = s_terminalRow;

			ConsoleOutputString("\r\n", false);
			/*s_terminalColumn = 0;
			s_terminalRow = 8;
			ConsoleOutput(skull_ascii_art, sizeof(skull_ascii_art), false);*/

			s_terminalColumn = x;
			s_terminalRow = y;
			printf_noFlush("Press any key to restart...\r\n");

			swapBuffers();
		}
		
		Pic(Pic::PIC1_CMD, Pic::PIC1_DATA).enableIrq(1);
		RegisterInterruptHandler(33, irq1);

		asm volatile("sti;"
			".byte 0xEB, 0xFD");
	}

	void printf_noFlush(CSTRING format, ...)
	{
		va_list list; va_start(list, format);
		_vprintf(consoleOutputCharacter, nullptr, format, list);
		va_end(list);
	}

	void printStackTrace(PVOID first, CSTRING prefix)
	{
		stack_frame* current = (stack_frame*)first;
		bool getFromEbp = false;
		if (!first)
		{
			current = reinterpret_cast<stack_frame*>(getEBP());
			getFromEbp = true;
		}
		if (first && !memory::HasVirtualAddress(first, 1))
		{
			current = reinterpret_cast<stack_frame*>(getEBP());
			getFromEbp = true;
		}
		int nStackFrames = 0;
		for (; current->down && memory::HasVirtualAddress(current->down, 1) && current->down != current; nStackFrames++, current = current->down);
		if (getFromEbp)
			current = reinterpret_cast<stack_frame*>(getEBP());
		else
			current = (stack_frame*)first;
		for (int i = nStackFrames; i > -1; i--, current = current->down)
		{
			STRING func = nullptr;
			SIZE_T functionAddress = 0;
			addr2func((PVOID)current->eip, func, functionAddress);
			printf_noFlush("%s%d: %p (%s+%d)\r\n", prefix, i, current->eip, func ? func : "[external code]", functionAddress ? (current->eip - functionAddress) : 0);
			if(func)
				delete func;
		}
	}
	void disassemble(PVOID _eip, CSTRING prefix)
	{
		if (!memory::HasVirtualAddress(_eip, 1))
		{
			printf_noFlush("%p: Invalid eip. Aborting...\r\n", _eip);
			return;
		}
		STRING filename = nullptr;

		addr2file(_eip, filename);
		printf_noFlush("Disassembly of address %p (%s):\r\n", _eip, filename ? filename : "Unknown file.");
		if(filename)
			delete filename;
#ifdef __i686__
		ZyanU32 eip = reinterpret_cast<ZyanU32>(_eip);
#else
		ZyanU64 eip = reinterpret_cast<ZyanU64>(_eip);
#endif

		PBYTE data = (PBYTE)_eip;

		ZyanUSize offset = 0;
		ZydisDisassembledInstruction instruction;

		int i = 0;

		while (ZYAN_SUCCESS(ZydisDisassembleIntel(
#ifdef __i686__
			ZYDIS_MACHINE_MODE_LEGACY_32,
#else
			ZYDIS_MACHINE_MODE_LONG_64,
#endif
			eip,
			data + offset,
			32,
			&instruction
		)))
		{
			if (i >= 10)
				break;
			STRING function = nullptr;
			SIZE_T functionAddress = 0;
			addr2func(reinterpret_cast<PVOID>(eip), function, functionAddress);
			printf_noFlush("%s%p (%s+%d): %s\r\n", prefix, eip, function ? function : "[external code]", functionAddress ? eip - functionAddress : 0, instruction.text);
			if (filename)
				delete function;
			offset += instruction.info.length;
			eip += instruction.info.length;
			i++;
		}
	}

	constexpr SIZE_T npos = 0xFFFFFFFF;

	/*SIZE_T findNext(CSTRING string, SIZE_T szStr, char ch, SIZE_T offset = 0)
	{
		SIZE_T i = offset;
		for (; i < szStr && string[i] != ch; i++);
		if (i == szStr)
			return npos;
		return i;
	}*/
	SIZE_T countTo(CSTRING string, CHAR ch)
	{
		SIZE_T i = 0;
		for (; string[i] != ch && string[i]; i++);
		return string[i] ? i : npos;
	}
	
	UINT32_T hex2bin(const char* str, unsigned size)
	{
		UINT32_T ret = 0;
		if (size > 8)
			return 0;
		str += *str == '\n';
		//unsigned size = utils::strlen(str);
		for (int i = size - 1, j = 0; i > -1; i--, j++)
		{
			char c = str[i];
			UINT32_T digit = 0;
			switch (c)
			{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				digit = c - '0';
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				digit = (c - 'A') + 10;
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				digit = (c - 'a') + 10;
				break;
			default:
				break;
			}
			/*if (!j)
			{
				ret = digit;
				continue;
			}*/
			ret |= digit << (j * 4);
		}
		return ret;
	}

	void addr2func(PVOID addr, STRING& str, SIZE_T& functionAddress)
	{
		UINTPTR_T address = GET_FUNC_ADDR(addr);
		CSTRING startAddress = reinterpret_cast<CSTRING>(reinterpret_cast<multiboot_module_t*>(g_multibootInfo->mods_addr)[4].mod_start);
		CSTRING endAddress = reinterpret_cast<CSTRING>(reinterpret_cast<multiboot_module_t*>(g_multibootInfo->mods_addr)[4].mod_end);
		for (CSTRING iter = startAddress;
			iter < endAddress;
			iter += countTo(iter, '\n') + 1)
		{
			CSTRING nextLine = nullptr;
			if(countTo(iter, '\n') != npos)
				nextLine = iter + countTo(iter, '\n') + 1;
			UINTPTR_T symbolAddress = hex2bin(iter, countTo(iter, ' '));
			UINTPTR_T nextSymbolAddress = 0;
			if (nextLine)
				nextSymbolAddress = hex2bin(nextLine, countTo(nextLine, ' '));
			else
				nextSymbolAddress = 0xFFFFFFFF;
			if (address >= symbolAddress && address < nextSymbolAddress)
			{
				iter += countTo(iter, ' ') + 3;
				SIZE_T size = countTo(iter, '\t');
				str = new CHAR[size + 1];
				utils::memcpy(str, iter, size);
				functionAddress = symbolAddress;
				break;
			}
		}
	}

	void addr2file(PVOID addr, STRING& str)
	{
		UINTPTR_T address = GET_FUNC_ADDR(addr);
		CSTRING startAddress = reinterpret_cast<CSTRING>(reinterpret_cast<multiboot_module_t*>(g_multibootInfo->mods_addr)[4].mod_start);
		CSTRING endAddress = reinterpret_cast<CSTRING>(reinterpret_cast<multiboot_module_t*>(g_multibootInfo->mods_addr)[4].mod_end);
		for (CSTRING iter = startAddress;
			iter < endAddress;
			iter += countTo(iter, '\n') + 1)
		{
			CSTRING nextLine = iter + countTo(iter, '\n') + 1;
			UINTPTR_T symbolAddress = hex2bin(iter, countTo(iter, ' '));
			UINTPTR_T nextSymbolAddress = hex2bin(nextLine, countTo(nextLine, ' '));
			if (address >= symbolAddress && address < nextSymbolAddress)
			{
				iter += countTo(iter, ' ') + 3;
				iter += countTo(iter, '\t') + 1;
				SIZE_T size = countTo(iter, ':');
				str = new CHAR[size + 1];
				utils::memcpy(str, iter, size);
				break;
			}
		}
	}


	static unsigned long int next = 1;  // NB: "unsigned long int" is assumed to be 32 bits wide

	int rand(void)  // RAND_MAX assumed to be 32767
	{
		next = next * 1103515245 + 12345;
		return (unsigned int)(next / 65536) % 32768;
	}

	void srand(unsigned int seed)
	{
		next = seed;
	}

	static inline UINT64_T rdtsc()
	{
		UINT64_T ret;
		asm volatile ("rdtsc" : "=A"(ret));
		return ret;
	}

	void poop()
	{
		extern UINT32_T* s_framebuffer;
		extern SIZE_T s_framebufferWidth;
		extern SIZE_T s_framebufferHeight;
		srand(rdtsc());
		for (UINTPTR_T i = 0, random = rand(); i < s_framebufferWidth * s_framebufferHeight; i++, random = rand())
		{
			s_framebuffer[i] = (rdtsc() % (random + 1)) * i;
			io_wait();
			io_wait();
			io_wait();
		}
	}
}
/*UINT32_T ipow(int n, int ex)
{
	UINT32_T res = n;
	for (UINT32_T i = 0; i < ex; i++) res *= n;
	res /= n;
	return res;
}*/