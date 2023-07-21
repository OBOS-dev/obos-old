/*
	gdt.c

	Copyright (c) 2023 Omar Berrow
*/

#include "gdt.h"

struct __gdt_ptr
{
	UINT16_T limit;
	UINT32_T base;
} __attribute__((packed));

_Static_assert(sizeof(struct __gdt_ptr) == 6, "sizeof(struct __gdt_ptr) is not six!");

static struct __gdt_ptr gdt_ptr;

typedef struct __gdtEntry
{
	UINT16_T limit_lowBytes;
	UINT16_T base_lowBytes;
	UINT8_T  base_middleByte;
	UINT8_T  access;
	UINT8_T	 granularity;
	UINT8_T base_highByte;
} __attribute__((packed)) gdtEntry;

static gdtEntry gdtEntries[6];

extern void gdtUpdate(UINT32_T address);

static void setGdtEntry(SIZE_T index, UINT32_T base, UINT32_T limit, UINT8_T access, UINT8_T granularity)
{
	gdtEntries[index].base_lowBytes = (base & 0xFFFF);
	gdtEntries[index].base_middleByte = (base >> 16) & 0xFF;
	gdtEntries[index].base_highByte = (base >> 24) & 0xFF;

	gdtEntries[index].limit_lowBytes = (limit & 0xFFFF);
	gdtEntries[index].granularity = (limit >> 16) & 0x0F;

	gdtEntries[index].granularity |= granularity & 0xF0;
	gdtEntries[index].access = access;
}

struct tss_entry
{
	UINT32_T prev_tss;
	UINT32_T esp0;
	UINT32_T ss0;
	UINT32_T esp1;
	UINT32_T ss1;
	UINT32_T esp2;
	UINT32_T ss2;
	UINT32_T cr3;
	UINT32_T eip;
	UINT32_T eflags;
	UINT32_T eax;
	UINT32_T ecx;
	UINT32_T edx;
	UINT32_T ebx;
	UINT32_T esp;
	UINT32_T ebp;
	UINT32_T esi;
	UINT32_T edi;
	UINT32_T es;
	UINT32_T cs;
	UINT32_T ss;
	UINT32_T ds;
	UINT32_T fs;
	UINT32_T gs;
	UINT32_T ldt;
	UINT16_T trap;
	UINT16_T iomap_base;
} __attribute__((packed));

static PVOID memset(PVOID buf, CHAR ch, SIZE_T size)
{
	for (int i = 0; i < size; ((PCHAR)buf)[i++] = ch);
	return buf;
}

struct tss_entry g_tssEntry;

void initGdt()
{
	memset(&g_tssEntry, 0, sizeof(g_tssEntry));

	asm volatile ("cli");

	gdt_ptr.limit = sizeof(gdtEntries) - 1;
	gdt_ptr.base = (UINTPTR_T)&gdtEntries;

	// 'NULL' Segment.
	setGdtEntry(0, 0, 0, 0, 0);
	// (Kernel) Code section.
	setGdtEntry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
	// (Kernel) Data section.
	setGdtEntry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
	// (User mode) Code section.
	setGdtEntry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
	// (User mode) Data section.
	setGdtEntry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

	// Make the tss entry.

	extern void tssUpdate();

	// TSS Entry.
	setGdtEntry(5, (UINTPTR_T)&g_tssEntry, (UINTPTR_T)&g_tssEntry + sizeof(g_tssEntry), 0xE9, 0x00);
	
	// Set the kernel's stack section.
	g_tssEntry.ss0 = 0x10;
	// The kernel's stack is nullptr for now.
	g_tssEntry.esp0 = 0x00;

	// Change to this (tss entry) from user mode.
	g_tssEntry.cs = (3 * 8) | 3;
	g_tssEntry.ss = g_tssEntry.ds = g_tssEntry.es = g_tssEntry.fs = g_tssEntry.gs = (4 * 8) | 3;

	gdtUpdate((UINTPTR_T)&gdt_ptr);
	tssUpdate();
}

void changeKernelStack(UINTPTR_T stack)
{
	g_tssEntry.esp0 = stack;
}