/*
	oboskrnl/klog.cpp

	Copyright (c) 2023-2024 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <console.h>
#include <atomic.h>
#include <memory_manipulation.h>
#include <utils/string.h>

#include <stdarg.h>

#include <multitasking/arch.h>

#include <multitasking/locks/mutex.h>

#include <allocators/liballoc.h>

#if (defined(__x86_64__) || defined(_WIN64))
#include <x86_64-utils/asm.h>
#include <arch/x86_64/irq/irq.h>
#include <arch/x86_64/interrupt.h>
namespace obos
{
	void EarlyKPanic();
	bool EnterSleepState(int sleepState);
	extern bool g_uacpiInitialized;
	extern bool g_unMask, g_halt;
}
#endif

#define STB_SPRINTF_NOFLOAT 1
#define STB_SPRINTF_IMPLEMENTATION 1
#define STB_SPRINTF_MIN 1
#include <utils/stb_sprintf.h>

//static char* itoa(intptr_t value, char* result, int base);
//static char* itoa_unsigned(uintptr_t value, char* result, int base);
//static char toupper(char ch)
//{
//	char tmp = ch - 'a';
//	if (tmp > 26 || tmp < 0)
//		return ch;
//	return tmp + 'A';
//}

namespace obos
{
	namespace logger
	{
		static bool isNumber(char ch)
		{
			char temp = ch - '0';
			return temp >= 0 && temp < 10;
		}
		static uint64_t dec2bin(const char* str, size_t size)
		{
			uint64_t ret = 0;
			for (size_t i = 0; i < size; i++)
			{
				if ((str[i] - '0') < 0 || (str[i] - '0') > 9)
					continue;
				ret *= 10;
				ret += str[i] - '0';
			}
			return ret;
		}
		static uint64_t hex2bin(const char* str, size_t size)
		{
			uint64_t ret = 0;
			str += *str == '\n';
			for (int i = size - 1, j = 0; i > -1; i--, j++)
			{
				char c = str[i];
				uintptr_t digit = 0;
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
				ret |= digit << (j * 4);
			}
			return ret;
		}
		static uint64_t oct2bin(const char* str, size_t size)
		{
			uint64_t n = 0;
			const char* c = str;
			while (size-- > 0)
				n = n * 8 + (uint64_t)(*c++ - '0');
			return n;
		}
		uint64_t strtoull(const char* str, char** endptr, int base)
		{
			while (!isNumber(*str++));
			if (!base)
			{
				base = 10;
				if (*(str - 1) == 'x' || *(str - 1) == 'X')
					base = 16;
				else if (*str == '0')
				{
					base = 8;
					str++;
				}
			}
			size_t sz = 0;
			while (isNumber(*str++))
				sz++;
			if (endptr)
				*endptr = (char*)(str + sz);
			switch (base)
			{
			case 10:
				return dec2bin(str, sz);
			case 16:
				return hex2bin(str, sz);
			case 8:
				return oct2bin(str, sz);
			default:
				break;
			}
			return 0xffff'ffff'ffff'ffff;
		}
#define inc_or_abort(str, ch, index) while((ch = str[++index]) == 0) break
		//size_t printf_impl(void(*printCallback)(char ch, void* userdata), void* userdata, const char* format, va_list list)
		//{
		//	size_t ret = 0;
		//	int32_t currentPadding = 0; // No character-padding by default for hex printing.
		//	for (size_t i = 0; format[i]; i++)
		//	{
		//		char ch = format[i];
		//		if (ch == '%')
		//		{
		//			char format_character = 0;
		//			inc_or_abort(format, format_character, i);
		//			switch (format_character)
		//			{
		//			case 'e':
		//				currentPadding = va_arg(list, intptr_t);
		//				break;
		//			case 'd':
		//			case 'i':
		//			{
		//				char str[12] = {};
		//				itoa(va_arg(list, intptr_t), str, 10);
		//				for (int i = 0; str[i]; i++, ret++)
		//					printCallback(str[i], userdata);
		//				break;
		//			}
		//			case 'u':
		//			{
		//				char str[12] = {};
		//				itoa_unsigned(va_arg(list, uintptr_t), str, 10);
		//				for (int i = 0; str[i]; i++, ret++)
		//					printCallback(str[i], userdata);
		//				break;
		//			}
		//			case 'x':
		//			{
		//				char str[sizeof(uintptr_t) * 2 + 1] = {};
		//				itoa_unsigned(va_arg(list, uintptr_t), str, 16);
		//				size_t size = utils::strlen(str);
		//				if ((intptr_t)(currentPadding - size) > 0)
		//				{
		//					for (size_t i = 0; i < (currentPadding - size); i++, ret++)
		//						printCallback('0', userdata);
		//				}
		//				for (size_t i = 0; i < size; i++, ret++)
		//					printCallback(str[i], userdata);
		//				break;
		//			}
		//			case 'X':
		//			{
		//				char str[sizeof(uintptr_t) * 2 + 1] = {};
		//				itoa_unsigned(va_arg(list, uintptr_t), str, 16);
		//				size_t size = utils::strlen(str);
		//				if ((intptr_t)(currentPadding - size) > 0)
		//				{
		//					for (size_t i = 0; i < (currentPadding - size); i++, ret++)
		//						printCallback('0', userdata);
		//				}
		//				for (size_t i = 0; i < size; i++, ret++)
		//					printCallback(toupper(str[i]), userdata);
		//				break;
		//			}
		//			case 'c':
		//			{
		//				printCallback((char)(va_arg(list, uintptr_t)), userdata);
		//				ret++;
		//				break;
		//			}
		//			case 's':
		//			{
		//				const char* str = va_arg(list, const char*);
		//				if (!str)
		//					break;
		//				for (int i = 0; str[i]; i++, ret++)
		//					printCallback(str[i], userdata);
		//				break;
		//			}
		//			case 'S':
		//			{
		//				utils::String& str = va_arg(list, utils::String);
		//				for (size_t i = 0; i < str.length(); i++, ret++)
		//					printCallback(str[i], userdata);
		//				break;
		//			}
		//			case 'p':
		//			{
		//				char str[sizeof(uintptr_t) * 2 + 1] = {};
		//				itoa_unsigned(va_arg(list, uintptr_t), str, 16);
		//				printCallback('0', userdata);
		//				printCallback('x', userdata);
		//				size_t size = 0;
		//				for (; str[size]; size++);
		//				for (size_t i = 0; i < (sizeof(uintptr_t) * 2 - size); i++)
		//					printCallback('0', userdata);
		//				for (size_t i = 0; i < size; i++)
		//					printCallback(str[i], userdata);
		//				ret += sizeof(uintptr_t) * 2 + 2;
		//				break;
		//			}
		//			// Do we really need this?
		//			/*case 'n':
		//			{
		//				signed int* ptr = va_arg(list, signed int*);
		//				*ptr = (signed int)ret;
		//				break;
		//			}*/
		//			case '%':
		//			{
		//				printCallback('%', userdata);
		//				ret++;
		//				break;
		//			}
		//			default:
		//				// Workaround to a bug
		//				va_arg(list, uintptr_t);
		//				break;
		//			}
		//			continue;
		//		}
		//		printCallback(ch, userdata);
		//		ret++;
		//	}
		//	return ret;
		//}

		static char* consoleOutputCallback(const char* buf, void* , int len)
		{
			for (size_t i = 0; i < (size_t)len; i++)
			{
				g_kernelConsole.ConsoleOutput(buf[i]);
#if E9_HACK && (defined(__x86_64__) || defined(_WIN64))
				outb(0xE9, buf[i]);
				outb(0x3f8, buf[i]);
#endif
			}
			return (char*)buf;
		}

		locks::Mutex printf_lock;
		size_t printf(const char* format, ...)
		{
			printf_lock.Lock();
			va_list list;
			va_start(list, format);
			char ch = 0;
			size_t ret = stbsp_vsprintfcb(consoleOutputCallback, nullptr, &ch, format, list);
			va_end(list);
			printf_lock.Unlock();
			return ret;
		}
		size_t vprintf(const char* format, va_list list)
		{
			printf_lock.Lock();
			char ch = 0;
			size_t ret = stbsp_vsprintfcb(consoleOutputCallback, nullptr, &ch, format, list);
			printf_lock.Unlock();
			return ret;
		}

		/*void sprintf_callback(char ch, void* userdata)
		{
			struct _data
			{
				char* str;
				size_t index;
			} *data = (_data*)userdata;
			if (data->str)
				data->str[data->index++] = ch;
		}*/
		size_t sprintf(char* dest, const char* format, ...)
		{
			if (!dest)
			{
				va_list list;
				va_start(list, format);
				size_t ret = stbsp_vsnprintf(dest, 0, format, list);
				va_end(list);
				return ret;
			}
			va_list list;
			va_start(list, format);
			size_t ret = stbsp_vsprintf(dest, format, list);
			va_end(list);
			return ret;
		}
		size_t vsprintf(char* dest, const char* format, va_list list)
		{
			if (!dest)
				return stbsp_vsnprintf(dest, 0, format, list);
			return stbsp_vsprintf(dest, format, list);
		}

		locks::Mutex debug_lock;
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

#ifdef OBOS_DEBUG
		size_t debug(const char* format, ...)
		{
			__impl_log(BLUE, DEBUG_PREFIX_MESSAGE, debug_lock);
			return 0;
		}
#else
		size_t debug(const char*, ...)
		{
			return 0;
		}
#endif
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
#if defined(__x86_64__) || defined(_WIN64)
			RegisterInterruptHandler(0x21, [](interrupt_frame*) {
				if (!g_uacpiInitialized)
					EarlyKPanic();
				else
					EnterSleepState(5);
				logger::warning("Software %s failed, manual shutdown is required (holding down the power button).\n", g_uacpiInitialized ? "shutdown" : "reboot");
				});
			MapIRQToVector(1, 0x21);
			// Tell the first core to enable interrupts.
			g_unMask = true;
			g_halt = false;
			SendIPI(DestinationShorthand::None, DeliveryMode::NMI, 2, 0);
			printf("Press any key to %s...\n", g_uacpiInitialized ? "shutdown" : "reboot");
#endif
			g_kernelConsole.SwapBuffers();
			thread::StopCPUs(true);
			while (1);
		}
	}
}

//// Credit: http://www.strudel.org.uk/itoa/
//static char* itoa(intptr_t value, char* result, int base) {
//	// check that the base if valid
//	if (base < 2 || base > 36) { *result = '\0'; return result; }
//
//	char* ptr = result, * ptr1 = result, tmp_char;
//	intptr_t tmp_value;
//
//	do {
//		tmp_value = value;
//		value /= base;
//		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"[35 + (tmp_value - value * base)];
//	} while (value);
//
//	// Apply negative sign
//	if (tmp_value < 0) *ptr++ = '-';
//	*ptr-- = '\0';
//	while (ptr1 < ptr) {
//		tmp_char = *ptr;
//		*ptr-- = *ptr1;
//		*ptr1++ = tmp_char;
//	}
//	return result;
//}
//static void strreverse(char* begin, int size)
//{
//	int i = 0;
//	char tmp = 0;
//
//	for (i = 0; i < size / 2; i++)
//	{
//		tmp = begin[i];
//		begin[i] = begin[size - i - 1];
//		begin[size - i - 1] = tmp;
//	}
//}
//// Credit: http://www.strudel.org.uk/itoa/
//static char* itoa_unsigned(uintptr_t value, char* result, int base)
//{
//	// check that the base if valid
//
//	if (base < 2 || base > 16)
//	{
//		*result = 0;
//		return result;
//	}
//
//	char* out = result;
//
//	uintptr_t quotient = value;
//
//	do
//	{
//
//		uintptr_t abs = /*(quotient % base) < 0 ? (-(quotient % base)) : */(quotient % base);
//
//		*out = "0123456789abcdef"[abs];
//
//		++out;
//
//		quotient /= base;
//
//	} while (quotient);
//
//	strreverse(result, out - result);
//
//	return result;
//
//}