/*
	oboskrnl/klog.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <stdarg.h>

#include <klog.h>

static char* itoa(intptr_t value, char* result, int base);
static char* itoa_unsigned(uintptr_t value, char* result, int base);
static char toupper(char ch)
{
	char tmp = ch - 'a';
	if (tmp > 26 || tmp < 0)
		return ch;
	return tmp + 'A';
}

namespace obos
{
	namespace logger
	{

		enum
		{
			GREY = 0xD3D3D3,
			GREEN = 0x03D12B,
			YELLOW = 0xffcc00,
			ERROR_RED = 0xcc3300,
			PANIC_RED = 0xac1616,
		};
		size_t printf_impl(void(*printCallback)(char ch, void* userdata), void* userdata, const char* format, va_list list)
		{
			size_t ret = 0;
			for (size_t i = 0; format[i]; i++)
			{
				char ch = format[i];
				if (ch == '%')
				{
					char format_character = format[++i];
					if (!format_character)
						break;
					switch (format_character)
					{
					case 'd':
					case 'i':
					{
						char str[12] = {};
						itoa(va_arg(list, intptr_t), str, 10);
						for (int i = 0; str[i]; i++, ret++)
							printCallback(str[i], userdata);
						break;
					}
					case 'u':
					{
						char str[12] = {};
						itoa_unsigned(va_arg(list, uintptr_t), str, 10);
						for (int i = 0; str[i]; i++, ret++)
							printCallback(str[i], userdata);
						break;
					}
					case 'x':
					{
						char str[17] = {};
						itoa_unsigned(va_arg(list, uintptr_t), str, 16);
						for (int i = 0; str[i]; i++, ret++)
							printCallback(str[i], userdata);
						break;
					}
					case 'X':
					{
						char str[17] = {};
						itoa_unsigned(va_arg(list, uintptr_t), str, 16);
						for (int i = 0; str[i]; i++, ret++)
							printCallback(toupper(str[i]), userdata);
						break;
					}
					case 'c':
					{
						printCallback((char)(va_arg(list, uintptr_t)), userdata);
						ret++;
						break;
					}
					case 's':
					{
						const char* str = va_arg(list, const char*);
						for (int i = 0; str[i]; i++, ret++)
							printCallback(str[i], userdata);
					}
					case 'p':
					{
						char str[sizeof(uintptr_t) * 2 + 1] = {};
						itoa_unsigned(va_arg(list, uintptr_t), str, 16);
						printCallback('0', userdata);
						printCallback('x', userdata);
						size_t size = 0;
						for (; str[size]; size++);
						for (size_t i = 0; i < ((sizeof(uintptr_t) * 2) - size); i++)
							printCallback('0', userdata);
						for (size_t i = 0; i < size; i++, ret++)
							printCallback(str[i], userdata);
						ret += sizeof(uintptr_t) * 2 + 2;
						break;
					}
					case 'n':
					{
						signed int* ptr = va_arg(list, signed int*);
						*ptr = (signed int)ret;
						break;
					}
					case '%':
					{
						printCallback('%', userdata);
						ret++;
						break;
					}
					default:
						break;
					}
					continue;
				}
				printCallback(ch, userdata);
				ret++;
			}
			return ret;
		}

		static void consoleOutputCallback(char ch, void*)
		{
			g_kernelConsole.ConsoleOutput(ch);
		}

		size_t printf(const char* format, ...)
		{
			va_list list;
			va_start(list, format);
			size_t ret = printf_impl(consoleOutputCallback, nullptr, format, list);
			va_end(list);
			return ret;
		}
		size_t vprintf(const char* format, va_list list)
		{
			return printf_impl(consoleOutputCallback, nullptr, format, list);
		}
		
#define __impl_log(colour, msg) \
			uint32_t oldForeground = 0;\
			uint32_t oldBackground = 0;\
			g_kernelConsole.GetColour(&oldForeground, &oldBackground);\
			g_kernelConsole.SetColour(colour, oldBackground);\
			va_list list; va_start(list, format);\
			size_t ret = printf(msg);\
			ret += vprintf(format, list);\
			va_end(list);\
			g_kernelConsole.SetColour(oldForeground, oldBackground);\
			return ret

		size_t log(const char* format, ...)
		{
			__impl_log(GREEN, "[Log] ");
		}
		size_t warning(const char* format, ...)
		{
			__impl_log(YELLOW, "[Warning] ");
		}
		size_t error(const char* format, ...)
		{
			__impl_log(ERROR_RED, "[Error] ");

		}
		[[noreturn]] void panic(const char* format, ...)
		{
			g_kernelConsole.SetPosition(0,0);
			g_kernelConsole.SetColour(GREY, PANIC_RED);
			va_list list;
			va_start(list, format);
			vprintf(format, list);
			va_end(list);
			stackTrace();
			while (1);
		}
	}
}

// Credit: http://www.strudel.org.uk/itoa/
static char* itoa(intptr_t value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, * ptr1 = result, tmp_char;
	intptr_t tmp_value;

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
static char* itoa_unsigned(uintptr_t value, char* result, int base)
{
	// check that the base if valid

	if (base < 2 || base > 16)
	{
		*result = 0;
		return result;
	}

	char* out = result;

	uintptr_t quotient = value;

	do
	{

		uintptr_t abs = /*(quotient % base) < 0 ? (-(quotient % base)) : */(quotient % base);

		*out = "0123456789abcdef"[abs];

		++out;

		quotient /= base;

	} while (quotient);

	strreverse(result, out - result);

	return result;

}