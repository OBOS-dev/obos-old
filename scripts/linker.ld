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

	_ZN4obos7__etextE = .;

	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}
	_ZN4obos9__erodataE = .;
 
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}
 
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
	
	_ZN4obos9endKernelE = .;
}