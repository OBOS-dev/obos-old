/*
	linker.ld
	
	Copyright (c) 2023 Omar Berrow
*/

ENTRY(_start)

SECTIONS
{
	. = 1M;
 
	_kernel_start = .;

    .multiboot.data : {
        *(.multiboot.data)
    }

    .multiboot.text : {
        *(.multiboot.text)
    }

	. += 0xC0000000;

	.text ALIGN (4K) : AT (ADDR (.text) - 0xC0000000)
	{
		*(.text)
	}
	.rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xC0000000)
	{
		*(.rodata)
	}
	_ZN4obos6memory9__erodataE = .;
	.data ALIGN (4K) : AT (ADDR (.data) - 0xC0000000)
	{
		*(.data)
	}
	.bss ALIGN (4K) : AT (ADDR (.bss) - 0xC0000000)
	{
		*(COMMON)
		*(.bss)
		*(.bootstrap_stack)
	}
	_kernel_end = .;
	
}
_ZN4obos6memory8endImageE = 0x400000;
_ZN4obos6memory9endKernelE = 0xE0000000;