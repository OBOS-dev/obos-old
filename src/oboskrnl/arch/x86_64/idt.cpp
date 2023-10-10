/*
	oboskrnl/arch/x86_64/idt.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include <int.h>
#include <memory_manipulation.h>

#include <arch/x86_64/interrupt.h>

#define DEFINE_ISR_FUNCTION(n) extern "C" void isr ##n();

DEFINE_ISR_FUNCTION(0);
DEFINE_ISR_FUNCTION(1);
DEFINE_ISR_FUNCTION(2);
DEFINE_ISR_FUNCTION(3);
DEFINE_ISR_FUNCTION(4);
DEFINE_ISR_FUNCTION(5);
DEFINE_ISR_FUNCTION(6);
DEFINE_ISR_FUNCTION(7);
DEFINE_ISR_FUNCTION(8);
DEFINE_ISR_FUNCTION(9);
DEFINE_ISR_FUNCTION(10);
DEFINE_ISR_FUNCTION(11);
DEFINE_ISR_FUNCTION(12);
DEFINE_ISR_FUNCTION(13);
DEFINE_ISR_FUNCTION(14);
DEFINE_ISR_FUNCTION(15);
DEFINE_ISR_FUNCTION(16);
DEFINE_ISR_FUNCTION(17);
DEFINE_ISR_FUNCTION(18);
DEFINE_ISR_FUNCTION(19);
DEFINE_ISR_FUNCTION(20);
DEFINE_ISR_FUNCTION(21);
DEFINE_ISR_FUNCTION(22);
DEFINE_ISR_FUNCTION(23);
DEFINE_ISR_FUNCTION(24);
DEFINE_ISR_FUNCTION(25);
DEFINE_ISR_FUNCTION(26);
DEFINE_ISR_FUNCTION(27);
DEFINE_ISR_FUNCTION(28);
DEFINE_ISR_FUNCTION(29);
DEFINE_ISR_FUNCTION(30);
DEFINE_ISR_FUNCTION(31);


namespace obos
{
	struct idtEntry
	{
		uint16_t offset1;
		uint16_t selector;
		uint8_t ist;
		uint8_t typeAttributes;
		uint16_t offset2;
		uint32_t offset3;
		uint32_t resv1;
	};
	struct idtPointer
	{
		uint16_t size;
		uintptr_t idt;
	} __attribute__((packed));

	extern "C" void idtFlush(idtPointer* idtptr);

	idtEntry g_idtEntries[256];
	idtPointer g_idtPointer = { sizeof(g_idtEntries) - 1, (uintptr_t)&g_idtEntries };
	void(*g_handlers[256])(interrupt_frame* frame);

	enum
	{
		DEFAULT_TYPE_ATTRIBUTE = 0x8E,
		TYPE_ATTRIBUTE_USER_MODE = 0x60
	};

	void _RegisterInterruptInIDT(int n, void(*handler)(), uint8_t typeAttributes)
	{
		utils::memzero(&g_idtEntries[n], sizeof(g_idtEntries[n]));
		g_idtEntries[n].ist = 0;
		g_idtEntries[n].typeAttributes = typeAttributes;
		g_idtEntries[n].selector = 0x08; // Kernel-Mode Code Segment
		
		uintptr_t base = (uintptr_t)handler;
		
		g_idtEntries[n].offset1 = base & 0xffff;
		g_idtEntries[n].offset2 = base >> 16;
		g_idtEntries[n].offset3 = base >> 32;
	}
#define RegisterInterruptInIDT(n) _RegisterInterruptInIDT(n, isr ##n, DEFAULT_TYPE_ATTRIBUTE | TYPE_ATTRIBUTE_USER_MODE)
	void InitializeIdt()
	{
		RegisterInterruptInIDT(0);
		RegisterInterruptInIDT(1);
		RegisterInterruptInIDT(2);
		RegisterInterruptInIDT(3);
		RegisterInterruptInIDT(4);
		RegisterInterruptInIDT(5);
		RegisterInterruptInIDT(6);
		RegisterInterruptInIDT(7);
		RegisterInterruptInIDT(8);
		RegisterInterruptInIDT(9);
		RegisterInterruptInIDT(10);
		RegisterInterruptInIDT(11);
		RegisterInterruptInIDT(12);
		RegisterInterruptInIDT(13);
		RegisterInterruptInIDT(14);
		RegisterInterruptInIDT(15);
		RegisterInterruptInIDT(16);
		RegisterInterruptInIDT(17);
		RegisterInterruptInIDT(18);
		RegisterInterruptInIDT(19);
		RegisterInterruptInIDT(20);
		RegisterInterruptInIDT(21);
		RegisterInterruptInIDT(22);
		RegisterInterruptInIDT(23);
		RegisterInterruptInIDT(24);
		RegisterInterruptInIDT(25);
		RegisterInterruptInIDT(26);
		RegisterInterruptInIDT(27);
		RegisterInterruptInIDT(28);
		RegisterInterruptInIDT(29);
		RegisterInterruptInIDT(30);
		RegisterInterruptInIDT(31);

		idtFlush(&g_idtPointer);
	}
	void RegisterInterruptHandler(byte interrupt, void(*handler)(interrupt_frame* frame))
	{
		g_handlers[interrupt] = handler;
	}
}