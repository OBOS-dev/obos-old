if grub-file --is-x86-multiboot out/oboskrnl; then
    echo Found multiboot header in out/oboskrnl
else
    echo Something is wrong. "out/oboskrnl," has no multiboot header.
    make clean
    exit
fi