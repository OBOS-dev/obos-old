/*
	kmain.cpp

	Copyright (c) 2023 Omar Berrow
*/

/*
* TODO: Port C11 and the C++ stdlib to the operating system.
* Port this C11 implementation:		   https://github.com/managarm/mlibc (MIT License).
* Port this C++ stdlib implementation: https://github.com/LiveMirror/stlport (Mysterious License, but it seems like it can work with the MIT license).
*/

#include <boot/multiboot.h>

#include <types.h>
#include <console.h>
#include <klog.h>
#include <inline-asm.h>

#include <descriptors/gdt/gdt.h>
#include <descriptors/idt/idt.h>

#include <memory_manager/physical.h>

#if !defined(__cplusplus) && !defined(__i686__) && !defined(__ELF__)
#error You must be using a C++ i686 compiler with elf. (i686-elf-g++, for example)
#endif

namespace obos
{
	multiboot_info_t* g_multibootInfo = nullptr;
	extern void* stack_top;

	// I just love name mangling.
	// _ZN4obos5kmainEP14multiboot_infoj
	void kmain(multiboot_info_t* header, DWORD magic)
	{
		if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
			return;
		obos::g_multibootInfo = header;

		// Initialize the console.
		obos::InitializeConsole(obos::ConsoleColor::LIGHT_GREY, obos::ConsoleColor::BLACK);

		obos::InitializeGdt();
		obos::InitializeIdt();

		/*obos::utils::Bitfield bitfield;
		bitfield.setBit(31);
		bitfield.setBit(15);
		bitfield.setBit(0);
		bitfield.setBit(4);*/
			
		obos::memory::InitializePhysicalMemoryManager();

		STRING base = (STRING)obos::memory::kalloc_physicalPages(4);
		base[0] = 'H';
		base[1] = 'e';
		base[2] = 'l';
		base[3] = 'l';
		base[4] = 'o';
		base[5] = '!';
		base[6] = '\0';
		obos::ConsoleOutputString(base);
	}
}

extern "C" void __cxa_pure_virtual()
{
	obos::ConsoleOutputString("Warning: Attempt to call a pure virtual function\r\n");
	asm volatile("cli;"
				 "hlt");

}