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

static gdtEntry gdtEntries[5];

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

void initGdt()
{
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

	gdtUpdate((UINTPTR_T)&gdt_ptr);
}