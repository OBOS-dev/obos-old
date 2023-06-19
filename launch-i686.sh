echo Launching the virutal machine.
qemu-system-i386 -drive file=release/obos.iso,format=raw -s -m 1G -mem-prealloc -serial file:./log.txt -serial tcp:0.0.0.0:1534,server