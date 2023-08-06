/*
	klog.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <klog.h>
#include <console.h>
#include <inline-asm.h>

#include <utils/memory.h>

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

		unsigned int abs = (quotient % base) < 0 ? (-(quotient % base)) : (quotient % base);

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


namespace obos
{
	static void(*s_consoleOutputCharacter)(BYTE ch, PVOID userdata) = [](BYTE ch, PVOID) 
	{ 
		obos::ConsoleOutputCharacter(ch, false);
		outb(0x3f8, ch);
	};
	void _vprintf(void(*printChar)(BYTE ch, PVOID userdata), PVOID printCharUserdata, CSTRING format, va_list list)
	{
		CSTRING _format = format;
		for (; *_format; _format++)
		{
			char ch = *_format;
			if(ch == '%')
			{
				ch = *++_format;
				switch (ch)
				{
				case 'd':
				case 'i':
				{
					char str[12];
					utils::ZeroMemory(str);
					itoa(va_arg(list, int), str, 10);
					for (int i = 0; str[i]; i++)
						printChar(str[i], printCharUserdata);
					break;
				}
				case 'u':
				{
					char str[12];
					utils::ZeroMemory(str);
					itoa_unsigned(va_arg(list, int), str, 10);
					for (int i = 0; str[i]; i++)
						printChar(str[i], printCharUserdata);
					break;
				}
				case 'o':
				{
					char str[13];
					utils::ZeroMemory(str);
					itoa_unsigned(va_arg(list, int), str, 8);
					for (int i = 0; str[i]; i++)
						printChar(str[i], printCharUserdata);
					break;
				}
				case 'x':
				{
					char str[9];
					utils::ZeroMemory(str);
					itoa_unsigned(va_arg(list, unsigned int), str, 16);
					for (int i = 0; str[i]; i++)
						printChar(str[i], printCharUserdata);
					break;
				}
				case 'X':
				{
					char str[9];
					utils::ZeroMemory(str);
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
					utils::ZeroMemory(str);
					itoa_unsigned(va_arg(list, unsigned int), str, 16);
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
		_vprintf(s_consoleOutputCharacter, nullptr, format, list);
		va_end(list);
		swapBuffers();
	}
	void vprintf(CSTRING format, va_list list)
	{
		_vprintf(s_consoleOutputCharacter, nullptr, format, list);
		swapBuffers();
	}

	void kpanic(CSTRING format, ...)
	{
		asm volatile("cli");
		EnterKernelSection();

		SetConsoleColor(0x00C0C0C0, 0x00FF0000);
		extern SIZE_T s_framebufferWidth;
		extern SIZE_T s_framebufferHeight;
		extern DWORD s_terminalRow;
		extern DWORD s_terminalColumn;
		extern UINT32_T* s_framebuffer;
		extern UINT32_T* s_backbuffer;
		utils::dwMemset(s_framebuffer, 0x00FF0000, s_framebufferWidth * s_framebufferHeight);
		utils::dwMemset(s_backbuffer, 0x00FF0000, s_framebufferWidth * s_framebufferHeight);
		s_terminalRow = 0;
		s_terminalColumn = 0;

		va_list list; va_start(list, format);
		_vprintf(s_consoleOutputCharacter, nullptr, format, list);
		va_end(list);

		swapBuffers();

		asm volatile("cli;"
					 "hlt");
	}
}
