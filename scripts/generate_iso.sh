clear
cd ..
echo Generating the iso \"obos.iso\"
# Copy the kernel to it's place.
cp out/oboskrnl isodir/obos/oboskrnl
grub-mkrescue -o out/obos.iso isodir
cd scripts