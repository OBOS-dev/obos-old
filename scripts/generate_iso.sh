cd ..
cp -u out/ahci isodir/obos/initrd/ahci
cp -u out/ps2Keyboard isodir/obos/initrd/ps2Keyboard
cp -u out/testProgram isodir/obos/initrd/testProgram
cd scripts
./make_initrd.sh
cd ..
echo Generating the iso \"obos.iso\"
# Copy the kernel to it's place.
cp -u out/oboskrnl isodir/obos/oboskrnl
# Copy the ps2Keyboard driver to it's place.
cp -u out/ps2Keyboard isodir/obos/drivers/input/ps2Keyboard
# Copy the ps2Keyboard driver to it's place.
cp -u out/ustarFilesystem isodir/obos/drivers/filesystems/ustarFilesystem
grub-mkrescue -o out/obos.iso isodir
cd scripts