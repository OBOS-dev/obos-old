#define ASM_FILE 1
#include "multiboot.h"

// Set the constants for the multiboot header.
.set FLAGS,    MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE
.set CHECKSUM, -(MULTIBOOT_HEADER_MAGIC + FLAGS)

// Declare a multiboot header. GRUB will now identify this program as a kernel.
.section .multiboot
.align 4
multiboot_header:
.long MULTIBOOT_HEADER_MAGIC
.long FLAGS
.long CHECKSUM
// This is need for some reason.
.long 0
.long 0
.long 0
.long 0
.long 0
// EGA Text mode.
.long 1
// 40x12 Characters
.long 40
.long 12
// BPP needs to be zero in text mode.
.long   0

// Set up the stack.
.section .bss
.align 16
stack_bottom:
.skip 16384 // 16 KiB
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    cli

    mov $stack_top, %esp
    xor %ebp, %ebp

//  Call the kernel's main.
    push %eax
    push %ebx
    call kmain

//  Hold the machine
    cli
    hlt
    ret