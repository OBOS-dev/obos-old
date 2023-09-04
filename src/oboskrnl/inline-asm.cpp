/*
    inline-asm.c

    Copyright (c) 2023 Omar Berrow
*/

#include <inline-asm.h>
#include <types.h>
#include <descriptors/idt/pic.h>

#include <multitasking/multitasking.h>

void outb(UINT16_T port, UINT8_T val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) :"memory");
}
void outw(UINT16_T port, UINT16_T val)
{
    asm volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) :"memory");
}
UINT8_T inb(UINT16_T port)
{
    volatile UINT8_T ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}
UINT16_T inw(UINT16_T port)
{
    volatile UINT16_T ret;
    asm volatile ( "inw %1, %0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}
void io_wait(void)
{
    outb(0x80, 0);
}

bool inKernelSection = false;

void cli()
{
    if (inKernelSection)
        return;
    asm volatile("cli");
}
void sti()
{
    if (inKernelSection)
        return;
    asm volatile("sti");
}
void hlt()
{
    asm volatile("hlt");
}

#define DEFINE_INT_CASE(intn) case intn: asm volatile("int $" #intn); break;

void _int(BYTE interrupt)
{
    switch (interrupt)
    {
       DEFINE_INT_CASE(0);
    case 1: asm volatile("int1"); break;
       DEFINE_INT_CASE(2);
    case 3: asm volatile("int3"); break;
    DEFINE_INT_CASE(4);
    DEFINE_INT_CASE(5);
    DEFINE_INT_CASE(6);
    DEFINE_INT_CASE(7);
    DEFINE_INT_CASE(8);
    DEFINE_INT_CASE(9);
    DEFINE_INT_CASE(10);
    DEFINE_INT_CASE(11);
    DEFINE_INT_CASE(12);
    DEFINE_INT_CASE(13);
    DEFINE_INT_CASE(14);
    DEFINE_INT_CASE(15);
    DEFINE_INT_CASE(16);
    DEFINE_INT_CASE(17);
    DEFINE_INT_CASE(18);
    DEFINE_INT_CASE(19);
    DEFINE_INT_CASE(20);
    DEFINE_INT_CASE(21);
    DEFINE_INT_CASE(22);
    DEFINE_INT_CASE(23);
    DEFINE_INT_CASE(24);
    DEFINE_INT_CASE(25);
    DEFINE_INT_CASE(26);
    DEFINE_INT_CASE(27);
    DEFINE_INT_CASE(28);
    DEFINE_INT_CASE(29);
    DEFINE_INT_CASE(30);
    DEFINE_INT_CASE(31);
    DEFINE_INT_CASE(32);
    DEFINE_INT_CASE(33);
    DEFINE_INT_CASE(34);
    DEFINE_INT_CASE(35);
    DEFINE_INT_CASE(36);
    DEFINE_INT_CASE(37);
    DEFINE_INT_CASE(38);
    DEFINE_INT_CASE(39);
    DEFINE_INT_CASE(40);
    DEFINE_INT_CASE(41);
    DEFINE_INT_CASE(42);
    DEFINE_INT_CASE(43);
    DEFINE_INT_CASE(44);
    DEFINE_INT_CASE(45);
    DEFINE_INT_CASE(46);
    DEFINE_INT_CASE(47);
    DEFINE_INT_CASE(48);
    DEFINE_INT_CASE(49);
    DEFINE_INT_CASE(50);
    DEFINE_INT_CASE(51);
    DEFINE_INT_CASE(52);
    DEFINE_INT_CASE(53);
    DEFINE_INT_CASE(54);
    DEFINE_INT_CASE(55);
    DEFINE_INT_CASE(56);
    DEFINE_INT_CASE(57);
    DEFINE_INT_CASE(58);
    DEFINE_INT_CASE(59);
    DEFINE_INT_CASE(60);
    DEFINE_INT_CASE(61);
    DEFINE_INT_CASE(62);
    DEFINE_INT_CASE(63);
    DEFINE_INT_CASE(64);
    DEFINE_INT_CASE(65);
    DEFINE_INT_CASE(66);
    DEFINE_INT_CASE(67);
    DEFINE_INT_CASE(68);
    DEFINE_INT_CASE(69);
    DEFINE_INT_CASE(70);
    DEFINE_INT_CASE(71);
    DEFINE_INT_CASE(72);
    DEFINE_INT_CASE(73);
    DEFINE_INT_CASE(74);
    DEFINE_INT_CASE(75);
    DEFINE_INT_CASE(76);
    DEFINE_INT_CASE(77);
    DEFINE_INT_CASE(78);
    DEFINE_INT_CASE(79);
    DEFINE_INT_CASE(80);
    DEFINE_INT_CASE(81);
    DEFINE_INT_CASE(82);
    DEFINE_INT_CASE(83);
    DEFINE_INT_CASE(84);
    DEFINE_INT_CASE(85);
    DEFINE_INT_CASE(86);
    DEFINE_INT_CASE(87);
    DEFINE_INT_CASE(88);
    DEFINE_INT_CASE(89);
    DEFINE_INT_CASE(90);
    DEFINE_INT_CASE(91);
    DEFINE_INT_CASE(92);
    DEFINE_INT_CASE(93);
    DEFINE_INT_CASE(94);
    DEFINE_INT_CASE(95);
    DEFINE_INT_CASE(96);
    DEFINE_INT_CASE(97);
    DEFINE_INT_CASE(98);
    DEFINE_INT_CASE(99);
    DEFINE_INT_CASE(100);
    DEFINE_INT_CASE(101);
    DEFINE_INT_CASE(102);
    DEFINE_INT_CASE(104);
    DEFINE_INT_CASE(105);
    DEFINE_INT_CASE(106);
    DEFINE_INT_CASE(107);
    DEFINE_INT_CASE(108);
    DEFINE_INT_CASE(109);
    DEFINE_INT_CASE(110);
    DEFINE_INT_CASE(111);
    DEFINE_INT_CASE(112);
    DEFINE_INT_CASE(113);
    DEFINE_INT_CASE(114);
    DEFINE_INT_CASE(115);
    DEFINE_INT_CASE(116);
    DEFINE_INT_CASE(117);
    DEFINE_INT_CASE(118);
    DEFINE_INT_CASE(119);
    DEFINE_INT_CASE(120);
    DEFINE_INT_CASE(121);
    DEFINE_INT_CASE(122);
    DEFINE_INT_CASE(123);
    DEFINE_INT_CASE(124);
    DEFINE_INT_CASE(125);
    DEFINE_INT_CASE(126);
    DEFINE_INT_CASE(127);
    DEFINE_INT_CASE(128);
    DEFINE_INT_CASE(129);
    DEFINE_INT_CASE(130);
    DEFINE_INT_CASE(131);
    DEFINE_INT_CASE(132);
    DEFINE_INT_CASE(133);
    DEFINE_INT_CASE(134);
    DEFINE_INT_CASE(135);
    DEFINE_INT_CASE(136);
    DEFINE_INT_CASE(137);
    DEFINE_INT_CASE(138);
    DEFINE_INT_CASE(139);
    DEFINE_INT_CASE(140);
    DEFINE_INT_CASE(141);
    DEFINE_INT_CASE(142);
    DEFINE_INT_CASE(143);
    DEFINE_INT_CASE(144);
    DEFINE_INT_CASE(145);
    DEFINE_INT_CASE(146);
    DEFINE_INT_CASE(147);
    DEFINE_INT_CASE(148);
    DEFINE_INT_CASE(149);
    DEFINE_INT_CASE(150);
    DEFINE_INT_CASE(151);
    DEFINE_INT_CASE(152);
    DEFINE_INT_CASE(153);
    DEFINE_INT_CASE(154);
    DEFINE_INT_CASE(155);
    DEFINE_INT_CASE(156);
    DEFINE_INT_CASE(157);
    DEFINE_INT_CASE(158);
    DEFINE_INT_CASE(159);
    DEFINE_INT_CASE(160);
    DEFINE_INT_CASE(161);
    DEFINE_INT_CASE(162);
    DEFINE_INT_CASE(163);
    DEFINE_INT_CASE(164);
    DEFINE_INT_CASE(165);
    DEFINE_INT_CASE(166);
    DEFINE_INT_CASE(167);
    DEFINE_INT_CASE(168);
    DEFINE_INT_CASE(169);
    DEFINE_INT_CASE(170);
    DEFINE_INT_CASE(171);
    DEFINE_INT_CASE(172);
    DEFINE_INT_CASE(173);
    DEFINE_INT_CASE(174);
    DEFINE_INT_CASE(175);
    DEFINE_INT_CASE(176);
    DEFINE_INT_CASE(177);
    DEFINE_INT_CASE(178);
    DEFINE_INT_CASE(179);
    DEFINE_INT_CASE(180);
    DEFINE_INT_CASE(181);
    DEFINE_INT_CASE(182);
    DEFINE_INT_CASE(183);
    DEFINE_INT_CASE(184);
    DEFINE_INT_CASE(185);
    DEFINE_INT_CASE(186);
    DEFINE_INT_CASE(187);
    DEFINE_INT_CASE(188);
    DEFINE_INT_CASE(189);
    DEFINE_INT_CASE(190);
    DEFINE_INT_CASE(191);
    DEFINE_INT_CASE(192);
    DEFINE_INT_CASE(193);
    DEFINE_INT_CASE(194);
    DEFINE_INT_CASE(195);
    DEFINE_INT_CASE(196);
    DEFINE_INT_CASE(197);
    DEFINE_INT_CASE(198);
    DEFINE_INT_CASE(199);
    DEFINE_INT_CASE(200);
    DEFINE_INT_CASE(201);
    DEFINE_INT_CASE(202);
    DEFINE_INT_CASE(204);
    DEFINE_INT_CASE(205);
    DEFINE_INT_CASE(206);
    DEFINE_INT_CASE(207);
    DEFINE_INT_CASE(208);
    DEFINE_INT_CASE(209);
    DEFINE_INT_CASE(210);
    DEFINE_INT_CASE(211);
    DEFINE_INT_CASE(212);
    DEFINE_INT_CASE(213);
    DEFINE_INT_CASE(214);
    DEFINE_INT_CASE(215);
    DEFINE_INT_CASE(216);
    DEFINE_INT_CASE(217);
    DEFINE_INT_CASE(218);
    DEFINE_INT_CASE(219);
    DEFINE_INT_CASE(220);
    DEFINE_INT_CASE(221);
    DEFINE_INT_CASE(222);
    DEFINE_INT_CASE(223);
    DEFINE_INT_CASE(224);
    DEFINE_INT_CASE(225);
    DEFINE_INT_CASE(226);
    DEFINE_INT_CASE(227);
    DEFINE_INT_CASE(228);
    DEFINE_INT_CASE(229);
    DEFINE_INT_CASE(230);
    DEFINE_INT_CASE(231);
    DEFINE_INT_CASE(232);
    DEFINE_INT_CASE(233);
    DEFINE_INT_CASE(234);
    DEFINE_INT_CASE(235);
    DEFINE_INT_CASE(236);
    DEFINE_INT_CASE(237);
    DEFINE_INT_CASE(238);
    DEFINE_INT_CASE(239);
    DEFINE_INT_CASE(240);
    DEFINE_INT_CASE(241);
    DEFINE_INT_CASE(242);
    DEFINE_INT_CASE(243);
    DEFINE_INT_CASE(244);
    DEFINE_INT_CASE(245);
    DEFINE_INT_CASE(246);
    DEFINE_INT_CASE(247);
    DEFINE_INT_CASE(248);
    DEFINE_INT_CASE(249);
    DEFINE_INT_CASE(250);
    DEFINE_INT_CASE(251);
    DEFINE_INT_CASE(252);
    DEFINE_INT_CASE(253);
    DEFINE_INT_CASE(254);
    DEFINE_INT_CASE(255);
    default:
        break;
    }
}

namespace obos
{
    int countCalled = 0;
    
    void EnterKernelSection()
    {
        Pic(Pic::PIC1_CMD, Pic::PIC1_DATA).disableIrq(0);
        asm volatile("cli");
        inKernelSection = true;
        countCalled++;
    }
    void LeaveKernelSection()
    {
        if (!countCalled)
            return;
        if (--countCalled)
            return;
        inKernelSection = false;
        asm volatile("sti");
        Pic(Pic::PIC1_CMD, Pic::PIC1_DATA).enableIrq(0);
    }
    void EnterSyscall()
    {
        multitasking::g_currentThread->isServicingSyscall = true;
    }
    void ExitSyscall()
    {
        multitasking::g_currentThread->isServicingSyscall = false;
    }
}