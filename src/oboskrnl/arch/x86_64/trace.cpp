/*
	oboskrnl/arch/x86_64/trace.cpp
	
	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <klog.h>
#include <limine.h>
#include <memory_manipulation.h>

namespace obos
{
	extern volatile limine_module_request module_request;
	namespace logger
	{
		constexpr size_t npos = 0xffffffffffffffff;
		size_t countTo(const char* string, char ch)
		{
			size_t i = 0;
			for (; string[i] != ch && string[i]; i++);
			return string[i] ? i : npos;
		}

		uintptr_t hex2bin(const char* str, unsigned size)
		{
			uintptr_t ret = 0;
			if (size > sizeof(uintptr_t) * 2)
				return 0;
			str += *str == '\n';
			//unsigned size = utils::strlen(str);
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
				/*if (!j)
				{
					ret = digit;
					continue;
				}*/
				ret |= digit << (j * 4);
			}
			return ret;
		}

		void addr2func(void* addr, char*& str, size_t& functionAddress, const char* startAddress, const char* endAddress)
		{
			uintptr_t address = reinterpret_cast<uintptr_t>(addr);
			for (const char* iter = startAddress;
				iter < endAddress;
				iter += countTo(iter, '\n') + 1)
			{
				const char* nextLine = nullptr;
				if (countTo(iter, '\n') != npos)
					nextLine = iter + countTo(iter, '\n') + 1;
				uintptr_t symbolAddress = hex2bin(iter, countTo(iter, ' '));
				uintptr_t nextSymbolAddress = 0;
				if (nextLine)
					nextSymbolAddress = hex2bin(nextLine, countTo(nextLine, ' '));
				else
					nextSymbolAddress = 0xFFFFFFFF;
				if (address >= symbolAddress && address < nextSymbolAddress)
				{
					iter += countTo(iter, ' ') + 3;
					size_t size = countTo(iter, '\t');
					str = new char[size + 1];
					utils::memcpy(str, iter, size);
					functionAddress = symbolAddress;
					break;
				}
			}
		}

		void addr2file(void* addr, char*& str, const char* startAddress, const char* endAddress)
		{
			uintptr_t address = reinterpret_cast<uintptr_t>(addr);
			for (const char* iter = startAddress;
				iter < endAddress;
				iter += countTo(iter, '\n') + 1)
			{
				const char* nextLine = iter + countTo(iter, '\n') + 1;
				uintptr_t symbolAddress = hex2bin(iter, countTo(iter, ' '));
				uintptr_t nextSymbolAddress = hex2bin(nextLine, countTo(nextLine, ' '));
				if (address >= symbolAddress && address < nextSymbolAddress)
				{
					iter += countTo(iter, ' ') + 3;
					iter += countTo(iter, '\t') + 1;
					size_t size = countTo(iter, ':');
					str = new char[size + 1];
					utils::memcpy(str, iter, size);
					break;
				}
			}
		}

		bool g_mapFileError = false;

		const char* getKernelMapFile(size_t* size)
		{
			if (g_mapFileError)
				return nullptr;
			const char* ret = nullptr;
			size_t moduleIndex = 0;

			for (; moduleIndex < module_request.response->module_count; moduleIndex++)
			{
				if (utils::strcmp(module_request.response->modules[moduleIndex]->path, "/obos/oboskrnl.map"))
				{
					ret = (char*)module_request.response->modules[moduleIndex]->address;
					if(size)
						*size = module_request.response->modules[moduleIndex]->size;
					break;
				}
			}
			if (!ret)
			{
				g_mapFileError = true;
				return nullptr;
			}
			return ret;
		}

		struct stack_frame
		{
			stack_frame* down;
			void* rip;
		};
		void stackTrace()
		{
			volatile stack_frame* frame = (volatile stack_frame*)__builtin_frame_address(0);
			size_t mapFileSize = 0;
			const char* mapFileStart = nullptr;
			if (!g_mapFileError)
				mapFileStart = getKernelMapFile(&mapFileSize);
			if (g_mapFileError || !mapFileStart)
				warning("Map file could not be found in the module list. Did you load it in the limine.cfg file?\n");
			printf("Stack trace:\n");
			int nFrames = -1;
			for (; frame; nFrames++) frame = frame->down;
			frame = (volatile stack_frame*)__builtin_frame_address(0);
			for (int i = nFrames; i != -1; i--)
			{
				if(!mapFileStart)
					printf("\t%d: %p\n", i, frame->rip);
				else
				{
					char* functionName = nullptr;
					uintptr_t functionStart = 0;
					addr2func(frame->rip, functionName, functionStart, mapFileStart, mapFileStart + mapFileSize);
					if(functionName)
					{
						printf("\t%d: %s+%d\n", i, functionName, (uintptr_t)frame->rip - functionStart);
						delete[] functionName;
					}
					else
						printf("\t%d: External Code (%p)\n", i, frame->rip);
				}
				frame = frame->down;
			}
		}
	}
}