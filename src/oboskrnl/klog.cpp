/*
	klog.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <klog.h>
#include <console.h>
#include <inline-asm.h>

#include <utils/memory.h>

#include <memory_manager/paging/allocate.h>

#include <descriptors/idt/idt.h>
#include <descriptors/idt/pic.h>

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
// This takes half a page of memory...
static constexpr const char skull_ascii_art[] =
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
"                 uuuuuuu\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "             uu$$$$$$$$$$$uu\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "          uu$$$$$$$$$$$$$$$$$uu\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "         u$$$$$$$$$$$$$$$$$$$$$u\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "        u$$$$$$$$$$$$$$$$$$$$$$$u\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "       u$$$$$$$$$$$$$$$$$$$$$$$$$u\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "       u$$$$$$$$$$$$$$$$$$$$$$$$$u\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "       u$$$$$$\"   \"$$$\"   \"$$$$$$u\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "       \"$$$$\"      u$u       $$$$\"\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "        $$$u       u$u       u$$$\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "        $$$u      u$$$u      u$$$\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "         \"$$$$uu$$$   $$$uu$$$$\"\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "          \"$$$$$$$\"   \"$$$$$$$\"\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "            u$$$$$$$u$$$$$$$u\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "             u$\"$\"$\"$\"$\"$\"$u\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "  uuu        $$u$ $ $ $ $u$$       uuu\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 " u$$$$        $$$$$u$u$u$$$       u$$$$\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "  $$$$$uu      \"$$$$$$$$$\"     uu$$$$$$\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "u$$$$$$$$$$$uu    \"\"\"\"\"    uuuu$$$$$$$$$$\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "$$$$\"\"\"$$$$$$$$$$uuu   uu$$$$$$$$$\"\"\"$$$\"\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 " \"\"\"      \"\"$$$$$$$$$$$uu \"\"$\"\"\"\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "           uuuu \"\"$$$$$$$$$$uuu\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "  u$$$uuu$$$$$$$$$uu \"\"$$$$$$$$$$$uuu$$$\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "  $$$$$$$$$$\"\"\"\"           \"\"$$$$$$$$$$$\"\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "   \"$$$$$\"                      \"\"$$$$\"\"\r\n"
"\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F\x0F"
 "     $$$\"                         $$$$\"\r\n"

;

namespace obos
{
	static void(*s_consoleOutputCharacter)(BYTE ch, PVOID userdata) = [](BYTE ch, PVOID) 
	{
		obos::ConsoleOutputCharacter(ch, false);
		outb(0x3f8, ch);
		outb(0xe9 , ch);
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

	static void irq1(const interrupt_frame*)
	{
		extern void RestartComputer();
		RestartComputer();
	}
	void kpanic(PVOID printStackTracePar, CSTRING format, ...)
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
			_vprintf(s_consoleOutputCharacter, nullptr, format, list);
			va_end(list);

			printf_noFlush("Stack trace:\r\n");
			printStackTrace(printStackTracePar, "\t");

			DWORD x = s_terminalColumn;
			DWORD y = s_terminalRow;

			ConsoleOutputString("\r\n", false);
			s_terminalColumn = 0;
			s_terminalRow = 8;
			ConsoleOutput(skull_ascii_art, sizeof(skull_ascii_art), false);
			
			s_terminalColumn = x;
			s_terminalRow = y;
			printf_noFlush("Press any key to restart...\r\n");
			Pic(Pic::PIC1_CMD, Pic::PIC1_DATA).enableIrq(1);
			RegisterInterruptHandler(33, irq1);

			swapBuffers();
		}

		asm volatile("sti;"
					 ".byte 0xEB, 0xFD");
	}

	void printf_noFlush(CSTRING format, ...)
	{
		va_list list; va_start(list, format);
		_vprintf(s_consoleOutputCharacter, nullptr, format, list);
		va_end(list);
	}

	void printStackTrace(PVOID first, CSTRING prefix)
	{
		stack_frame* current = (stack_frame*)first;
		if (!first)
			asm volatile ("mov %%ebp, %0" : : "memory"(current) : );
		if (first && !memory::HasVirtualAddress(first, 1))
			asm volatile ("mov %%ebp, %0" : : "memory"(current) : );
		int nStackFrames = 0;
		for (; current->down && memory::HasVirtualAddress(current->down, 1); nStackFrames++, current = current->down);
		asm volatile ("mov %%ebp, %0" : : "memory"(current) : );
		for (int i = nStackFrames; i > -1; i--, current = current->down)
			printf_noFlush("%s%d: %p\r\n", prefix, i, current->eip);
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
