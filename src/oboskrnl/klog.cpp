/*
	oboskrnl/klog.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <atomic.h>
#include <memory_manipulation.h>

#include <stdarg.h>

#include <multitasking/arch.h>

#include <multitasking/locks/mutex.h>

#include <liballoc/liballoc.h>

#if defined(__x86_64__) && defined(E9_HACK)
#include <x86_64-utils/asm.h>
#endif

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
		size_t printf_impl(void(*printCallback)(char ch, void* userdata), void* userdata, const char* format, va_list list)
		{
			size_t ret = 0;
			int32_t currentPadding = 0; // No character-padding by default for hex printing.
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
					case 'e':
						currentPadding = va_arg(list, intptr_t);
						break;
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
						char str[sizeof(uintptr_t) * 2 + 1] = {};
						itoa_unsigned(va_arg(list, uintptr_t), str, 16);
						size_t size = utils::strlen(str);
						if ((intptr_t)(currentPadding - size) > 0)
						{
							for (size_t i = 0; i < (currentPadding - size); i++, ret++)
								printCallback('0', userdata);
						}
						for (size_t i = 0; i < size; i++, ret++)
							printCallback(str[i], userdata);
						break;
					}
					case 'X':
					{
						char str[sizeof(uintptr_t) * 2 + 1] = {};
						itoa_unsigned(va_arg(list, uintptr_t), str, 16);
						size_t size = utils::strlen(str);
						if ((intptr_t)(currentPadding - size) > 0)
						{
							for (size_t i = 0; i < (currentPadding - size); i++, ret++)
								printCallback('0', userdata);
						}
						for (size_t i = 0; i < size; i++, ret++)
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
						if (!str)
							break;
						for (int i = 0; str[i]; i++, ret++)
							printCallback(str[i], userdata);
						break;
					}
					case 'p':
					{
						char str[sizeof(uintptr_t) * 2 + 1] = {};
						itoa_unsigned(va_arg(list, uintptr_t), str, 16);
						printCallback('0', userdata);
						printCallback('x', userdata);
						size_t size = 0;
						for (; str[size]; size++);
						for (size_t i = 0; i < (sizeof(uintptr_t) * 2 - size); i++)
							printCallback('0', userdata);
						for (size_t i = 0; i < size; i++)
							printCallback(str[i], userdata);
						ret += sizeof(uintptr_t) * 2 + 2;
						break;
					}
					// Do we really need this?
					/*case 'n':
					{
						signed int* ptr = va_arg(list, signed int*);
						*ptr = (signed int)ret;
						break;
					}*/
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
#if E9_HACK && defined(__x86_64__)
			outb(0xE9, ch);
#endif
		}

		locks::Mutex printf_lock;
		size_t printf(const char* format, ...)
		{
			printf_lock.Lock();
			va_list list;
			va_start(list, format);
			size_t ret = printf_impl(consoleOutputCallback, nullptr, format, list);
			va_end(list);
			printf_lock.Unlock();
			return ret;
		}
		size_t vprintf(const char* format, va_list list)
		{
			printf_lock.Lock();
			size_t ret = printf_impl(consoleOutputCallback, nullptr, format, list);
			printf_lock.Unlock();
			return ret;
		}

		void sprintf_callback(char ch, void* userdata)
		{
			struct _data
			{
				char* str;
				size_t index;
			} *data = (_data*)userdata;
			if (data->str)
				data->str[data->index++] = ch;
		}
		size_t sprintf(char* dest, const char* format, ...)
		{
			struct _data
			{
				char* str;
				size_t index;
			} data = { dest, 0 };
			va_list list;
			va_start(list, format);
			size_t ret = printf_impl(sprintf_callback, &data, format, list);
			va_end(list);
			return ret;
		}


		locks::Mutex log_lock;
		locks::Mutex warning_lock;
		locks::Mutex error_lock;
#define __impl_log(colour, msg, lock_name) \
			lock_name.Lock();\
			uint32_t oldForeground = 0;\
			uint32_t oldBackground = 0;\
			g_kernelConsole.GetColour(&oldForeground, &oldBackground);\
			g_kernelConsole.SetColour(colour, oldBackground);\
			while(*format == '\n') { printf("\n"); format++; } \
			va_list list; va_start(list, format);\
			size_t ret = printf(msg);\
			ret += vprintf(format, list);\
			va_end(list);\
			g_kernelConsole.SetColour(oldForeground, oldBackground);\
			lock_name.Unlock();\
			return ret

		size_t log(const char* format, ...)
		{
			__impl_log(GREEN, LOG_PREFIX_MESSAGE, log_lock);
		}
		size_t info(const char* format, ...)
		{
			__impl_log(GREEN, INFO_PREFIX_MESSAGE, log_lock);
		}
		size_t warning(const char* format, ...)
		{
			__impl_log(YELLOW, WARNING_PREFIX_MESSAGE, warning_lock);
		}
		size_t error(const char* format, ...)
		{
			__impl_log(ERROR_RED, ERROR_PREFIX_MESSAGE, error_lock);

		}
		void panic(void* stackTraceParameter, const char* format, ...)
		{
			va_list list;
			va_start(list, format);
			panicVariadic(stackTraceParameter, format, list);
			va_end(list); // shouldn't get hit
		}
		void panicVariadic(void* stackTraceParameter, const char* format, va_list list)
		{
			thread::StopCPUs(false);
			printf_lock.Unlock();
			g_kernelConsole.Unlock();
			thread::stopTimer();
			g_kernelConsole.SetPosition(0, 0);
			g_kernelConsole.SetColour(GREY, PANIC_RED, true);
			vprintf(format, list);
			if (CanAllocateMemory())
				stackTrace(stackTraceParameter);
			else
				warning("No stack trace avaliable.");
			thread::StopCPUs(true);
			while (1);
		}
		static void dumpAddr_impl(int a, ...)
		{
#ifdef __x86_64__
			va_list list;
			va_start(list, a);
			printf_impl([](char ch, void*) { outb(0xE9, ch); }, nullptr, "%p: %p\n", list);
			va_end(list);
#endif
		}
		void dumpAddr(uint32_t* addr)
		{
			dumpAddr_impl(0, addr, *addr);
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
static void strreverse(char* begin, int size)
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