/*
	idt.cpp

	Copyright (c) 2023 Omar Berrow
*/

#include "idt.h"


extern "C" void isr0();
extern "C" void isr1();
extern "C" void isr2();
extern "C" void isr3();
extern "C" void isr4();
extern "C" void isr5();
extern "C" void isr6();
extern "C" void isr7();
extern "C" void isr8();
extern "C" void isr9();
extern "C" void isr10();
extern "C" void isr11();
extern "C" void isr12();
extern "C" void isr13();
extern "C" void isr14();
extern "C" void isr15();
extern "C" void isr16();
extern "C" void isr17();
extern "C" void isr18();
extern "C" void isr19();
extern "C" void isr20();
extern "C" void isr21();
extern "C" void isr22();
extern "C" void isr23();
extern "C" void isr24();
extern "C" void isr25();
extern "C" void isr26();
extern "C" void isr27();
extern "C" void isr28();
extern "C" void isr29();
extern "C" void isr30();
extern "C" void isr31();
extern "C" void isr32();
extern "C" void isr33();
extern "C" void isr34();
extern "C" void isr35();
extern "C" void isr36();
extern "C" void isr37();
extern "C" void isr38();
extern "C" void isr39();
extern "C" void isr40();
extern "C" void isr41();
extern "C" void isr42();
extern "C" void isr43();
extern "C" void isr44();
extern "C" void isr45();
extern "C" void isr46();
extern "C" void isr47();

namespace obos
{
	extern void idtFlush(UINTPTR_T base);
	extern void(*g_interruptHandlers[256])(const obos::interrupt_frame* frame);
	struct IdtPointer
	{
		UINT16_T limit;
		UINT32_T base;
	} __attribute__((packed));

	IdtEntry::IdtEntry(UINT32_T base, UINT16_T sel, UINT8_T flags)
	{
		m_baseLow = base & 0xFFFF;
		m_baseHigh = (base >> 16) & 0xFFFF;

		m_sel = sel;
		m_always0 = 0;
		m_flags = flags | 0x60;
	}
	static IdtEntry   s_idtEntries[256];
	static IdtPointer s_idtPointer;

#define REGISTER_INTERRUPT(interrupt) \
s_idtEntries[interrupt] = IdtEntry((UINTPTR_T)isr ##interrupt, 0x08, 0x8E);

	void InitializeIdt()
	{
		REGISTER_INTERRUPT(0);
		REGISTER_INTERRUPT(1);
		REGISTER_INTERRUPT(2);
		REGISTER_INTERRUPT(3);
		REGISTER_INTERRUPT(4);
		REGISTER_INTERRUPT(5);
		REGISTER_INTERRUPT(6);
		REGISTER_INTERRUPT(7);
		REGISTER_INTERRUPT(8);
		REGISTER_INTERRUPT(9);
		REGISTER_INTERRUPT(10);
		REGISTER_INTERRUPT(11);
		REGISTER_INTERRUPT(12);
		REGISTER_INTERRUPT(13);
		REGISTER_INTERRUPT(14);
		REGISTER_INTERRUPT(15);
		REGISTER_INTERRUPT(16);
		REGISTER_INTERRUPT(17);
		REGISTER_INTERRUPT(18);
		REGISTER_INTERRUPT(19);
		REGISTER_INTERRUPT(20);
		REGISTER_INTERRUPT(21);
		REGISTER_INTERRUPT(22);
		REGISTER_INTERRUPT(23);
		REGISTER_INTERRUPT(24);
		REGISTER_INTERRUPT(25);
		REGISTER_INTERRUPT(26);
		REGISTER_INTERRUPT(27);
		REGISTER_INTERRUPT(28);
		REGISTER_INTERRUPT(29);
		REGISTER_INTERRUPT(30);
		REGISTER_INTERRUPT(31);
		REGISTER_INTERRUPT(32);
		REGISTER_INTERRUPT(33);
		REGISTER_INTERRUPT(34);
		REGISTER_INTERRUPT(35);
		REGISTER_INTERRUPT(36);
		REGISTER_INTERRUPT(37);
		REGISTER_INTERRUPT(38);
		REGISTER_INTERRUPT(39);
		REGISTER_INTERRUPT(40);
		REGISTER_INTERRUPT(41);
		REGISTER_INTERRUPT(42);
		REGISTER_INTERRUPT(43);
		REGISTER_INTERRUPT(44);
		REGISTER_INTERRUPT(45);
		REGISTER_INTERRUPT(46);
		REGISTER_INTERRUPT(47);

		s_idtPointer.limit = sizeof(s_idtEntries) - 1;
		s_idtPointer.base  = (UINTPTR_T)&s_idtEntries;
		idtFlush((UINTPTR_T)&s_idtPointer);
	}
	void RegisterInterruptHandler(UINT8_T interrupt, void(*isr)(const interrupt_frame* frame))
	{
		g_interruptHandlers[interrupt] = isr;
	}
}
//extern "C" void defaultInterruptHandler(const obos::interrupt_frame* frame)
//{
//	g_interruptHandlers[frame->intNumber](frame);
//}