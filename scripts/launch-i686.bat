@echo off
cd ..
echo Launching the virutal machine.
@rem Add -debugcon file:log.txt to use debugcon.
qemu-system-i386 -cdrom out/obos.iso ^
-drive id=disk,file=disk.img,if=none,format=raw ^
-device ahci,id=ahci ^
-device ide-hd,drive=disk,bus=ahci.0 ^
-boot order=d ^
-s ^
-m 1G ^
-serial tcp:0.0.0.0:1535,server,nowait ^
-serial tcp:0.0.0.0:1534,server,nowait ^ 
-monitor stdio ^
-d guest_errors
cd scripts