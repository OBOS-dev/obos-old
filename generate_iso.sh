echo Generating the iso \"obos.iso\"
./check_multiboot.sh
cp out/oboskrnl isodir/boot/oboskrnl.out
cp grub.cfg isodir/boot/grub/grub.cfg
cp obos_icon.ico isodir/obos_icon.ico
cp autorun.inf isodir/autorun.inf
rm release/obos.iso
grub-mkrescue -o release/obos.iso isodir