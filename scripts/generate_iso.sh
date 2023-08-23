cd ..
cp -u out/nvme isodir/obos/initrd/nvme
objcopy -S isodir/obos/initrd/nvme
cp -u out/ps2Keyboard isodir/obos/initrd/ps2Keyboard
objcopy -S isodir/obos/initrd/ps2Keyboard
nm out/oboskrnl --demangle=gnu-v3 -ln | grep -w --ignore-case T > isodir/obos/oboskrnl.map
cd scripts
./make_initrd.sh
cd ..
echo Generating the iso \"obos.iso\"
# Copy the kernel to it's place.
cp -u out/oboskrnl isodir/obos/oboskrnl
objcopy -S isodir/obos/oboskrnl
# Copy the ps2Keyboard driver to it's place.
cp -u out/ps2Keyboard isodir/obos/drivers/input/ps2Keyboard
# Copy the initrd driver to it's place.
cp -u out/ustarFilesystem isodir/obos/drivers/filesystems/ustarFilesystem
grub-mkrescue -o out/obos.iso isodir
cd scripts