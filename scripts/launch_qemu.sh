cd ../

qemu-system-x86_64 \
-drive file=out/obos.iso,format=raw \
-s -S \
-m 1G \
-drive if=pflash,format=raw,unit=0,file=ovmf/OVMF_CODE_4M.fd,readonly=on \
-drive if=pflash,format=raw,unit=1,file=ovmf/OVMF_VARS_4M.fd \
-cpu qemu64,+nx \
-monitor stdio \
-debugcon file:stdio \
-smp cores=2,threads=1,sockets=1 \
-M smm=off \
-d int \
-D qemu_log.txt
# -no-reboot
# rem -no-shutdown

cd scripts