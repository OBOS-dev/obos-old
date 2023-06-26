// Set the constants for the multiboot header.
.set ALIGN,    1<<0
.set MEMINFO,  1<<1
.set FLAGS,    ALIGN | MEMINFO
.set MAGIC,    0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

// Declare a multiboot header. GRUB will now identify this program as a kernel.
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

// Set up the stack.
.section .bss
.align 16
stack_bottom:
.skip 16384 // 16 KiB
stack_top:

.section .text
.global _start
.global _int
.type _start, @function
.type _int, @function
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