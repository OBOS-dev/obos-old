@echo off

cd ../

qemu-system-x86_64 ^
-drive file=out/obos.iso,format=raw ^
-s ^
-m 1G ^
-cpu qemu64,+nx

cd scripts