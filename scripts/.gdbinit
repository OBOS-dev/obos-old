set architecture i386:x86-64
target remote :1234
b *0x80
c
file ../out/oboskrnl