@echo off
echo Launching the virutal machine.
qemu-system-i386 -hda emulator_files/disk.img -cdrom release/obos.iso -boot order=d -s -m 1G -mem-prealloc -serial file:./log.txt -serial tcp:0.0.0.0:1534,server -no-shutdown -monitor stdio