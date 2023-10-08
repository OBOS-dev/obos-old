@echo off
cd ..
echo Launching the virutal machine.
qemu-system-x86_64 -cdrom out/obos.iso ^
-boot order=d ^
-s ^
-m 1G ^
-serial tcp:0.0.0.0:1535,server,nowait ^
-serial tcp:0.0.0.0:1534,server,nowait ^
-monitor stdio ^
-debugcon file:log.txt ^
-D qemu_log.txt ^
-d int ^
-M q35,smm=off ^
-cpu qemu64,+nx
@rem -D qemu_log.txt ^
@rem -d int ^
@rem -M q35,smm=off ^
@rem -drive file=disk.img,if=none,id=nvm,format=raw ^
@rem -device nvme,drive=nvm,serial=0,use-intel-id=on ^
cd scripts