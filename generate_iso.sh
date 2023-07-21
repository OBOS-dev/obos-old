clear
echo Generating the iso \"obos.iso\"
./check_multiboot.sh
# Copy the grub files.
mkdir isodir
cd isodir
mkdir boot
cd boot
mkdir grub
cd ../../
cp out/obos_fat archive_dir/drivers/obos_fat
cd archive_dir
tar -cf ../initrd.tar *
cd ..
cp initrd.tar isodir/initrd.tar
cp out/oboskrnl isodir/boot/oboskrnl.out
cp grub.cfg isodir/boot/grub/grub.cfg
cp obos_icon.ico isodir/obos_icon.ico
cp autorun.inf isodir/autorun.inf
grub-mkrescue -o release/obos.iso isodir