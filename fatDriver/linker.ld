/*
	linker.ld
	
	Copyright (c) 2023 Omar Berrow
*/

ENTRY(_start)
 
SECTIONS
{
	. = 10M;
 
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.text)
	}

	__etext = .;

	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}
 
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}
 
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
	
	endDriver = .;
}