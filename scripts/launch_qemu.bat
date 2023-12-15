@echo off

cd ../

del qemu_log.txt

qemu-system-x86_64 ^
-drive file=out/obos.iso,format=raw ^
-gdb tcp:0.0.0.0:1234 -S ^
-m 1G ^
-drive if=pflash,format=raw,unit=0,file=ovmf/OVMF_CODE_4M.fd,readonly=on ^
-drive if=pflash,format=raw,unit=1,file=ovmf/OVMF_VARS_4M.fd ^
-cpu qemu64,+nx,+pdpe1gb,+syscall,+fsgsbase,+rdrand,+rdseed,+rdtscp ^
-monitor stdio ^
-debugcon file:CON ^
-serial tcp:0.0.0.0:1534,server,nowait ^
-smp cores=4,threads=1,sockets=1 ^
-M smm=off ^
-d int ^
-D qemu_log.txt
rem -no-reboot
rem -no-shutdown

cd scripts