@echo off

cd ../

del qemu_log.txt

qemu-system-x86_64 ^
-drive id=disk1,file=out/obos.iso,if=none,format=raw -device ahci,id=ahci1 -device ide-hd,drive=disk1,bus=ahci1.0 ^
-drive id=disk2,file=disk.img,if=none,format=raw -device ide-hd,drive=disk2,bus=ahci1.1 ^
-gdb tcp:0.0.0.0:1234 -S ^
-m 1G ^
-cpu qemu64,+nx,+pdpe1gb,+syscall,+fsgsbase,+rdrand,+rdseed,+rdtscp,+smep,+smap,+sse,+sse2,+sse4.1,+sse4.2,+sse4a,+ssse3 ^
-monitor stdio ^
-debugcon file:CON ^
-serial tcp:0.0.0.0:1534,server,nowait ^
-smp cores=4,threads=1,sockets=1 ^
-M smm=off ^
-d trace:*ahci*,int ^
-D qemu_log.txt
rem  ^
rem -no-reboot
rem -no-shutdown
rem -drive if=pflash,format=raw,unit=0,file=ovmf/OVMF_CODE_4M.fd,readonly=on ^
rem -drive if=pflash,format=raw,unit=1,file=ovmf/OVMF_VARS_4M.fd ^

cd scripts