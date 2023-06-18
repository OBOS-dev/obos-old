# export "COMPILE_OPTIONS_RELEASE=-c -O3 -Wall -Werror -Wno-error=misleading-indentation -Wno-builtin-declaration-mismatch -DNDEBUG=1 -D_DEBUG=0"
# export "OBJECT_FILES=int/kernel_bootstrap.o int/kmain.o int/terminal.o int/acpi.o int/kalloc.o int/inline-asm.o int/klog.o int/kserial.o"
# echo Assembling "kernel/kernel_bootstrap-i686.S"
# i686-elf-as kernel/kernel_bootstrap-i686.S -o int/kernel_bootstrap.o
# echo Compiling "kernel/kmain.c"
# i686-elf-gcc kernel/kmain.c -o int/kmain.o $COMPILE_OPTIONS_RELEASE
# echo Compiling "kernel/terminal.c"
# i686-elf-gcc kernel/terminal.c -o int/terminal.o $COMPILE_OPTIONS_RELEASE
# echo Compiling "kernel/acpi.c"
# i686-elf-gcc kernel/acpi.c -o int/acpi.o $COMPILE_OPTIONS_RELEASE
# echo Compiling "kernel/kalloc.c"
# i686-elf-gcc kernel/kalloc.c -o int/kalloc.o $COMPILE_OPTIONS_RELEASE
# echo Compiling "kernel/inline-asm.c"
# i686-elf-gcc kernel/inline-asm.c -o int/inline-asm.o $COMPILE_OPTIONS_RELEASE
# echo Compiling "kernel/klog.c"
# i686-elf-gcc kernel/klog.c -o int/klog.o $COMPILE_OPTIONS_RELEASE
# echo Compiling "kernel/kserial.c"
# i686-elf-gcc kernel/kserial.c -o int/kserial.o $COMPILE_OPTIONS_RELEASE
# echo Linking object files...
# i686-elf-ld -T int/linker.ld $OBJECT_FILES -o bin/oboskrnl.out -nostdlib
# if grub-file --is-x86-multiboot bin/oboskrnl.out; then
#     echo Found multiboot header in oboskrnl.out
# else
#     echo Something is wrong. "oboskrnl.out," has no multiboot header.
#     rm int/*.o
#     rm bin/*.out
#     exit
# fi
cmake ./ -DCMAKE_BUILD_TYPE=Release -DLINKER_SCRIPT=int/linker.ld -G Ninja; ninja -j 0;