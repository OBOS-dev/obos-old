@echo off
cd ..
echo Launching the virutal machine.
qemu-system-i386 -cdrom out/obos.iso -boot order=d -s -m 1G -serial tcp:0.0.0.0:1535,server,nowait -serial tcp:0.0.0.0:1534,server,nowait -no-shutdown -monitor stdio -d guest_errors -debugcon file:log.txt
cd scripts