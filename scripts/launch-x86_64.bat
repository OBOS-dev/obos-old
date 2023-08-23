@echo off
cd ..
echo Launching the virutal machine.
@rem Add -debugcon file:log.txt to use debugcon.
qemu-system-x86_64 -cdrom out/obos.iso ^
-drive file=disk.img,if=none,id=nvm,format=raw ^
-device nvme,drive=nvm,serial=0,use-intel-id=on ^
-boot order=d ^
-s ^
-m 1G ^
-serial tcp:0.0.0.0:1535,server,nowait ^
-serial tcp:0.0.0.0:1534,server,nowait ^
-monitor stdio ^
-D qemu_log.txt ^
-debugcon file:log.txt
cd scripts