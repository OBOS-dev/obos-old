/*
	linker.ld
	
	Copyright (c) 2023 Omar Berrow
*/

ENTRY(_start)

SECTIONS
{
	. = 1M;
 
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(.text)
	}

	_ZN4obos6memory7__etextE = .;

	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}
	_ZN4obos6memory9__erodataE = .;
 
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}
 
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
	
}
_ZN4obos6memory8endImageE = 0x800000;
_ZN4obos6memory9endKernelE = 0x1100000;