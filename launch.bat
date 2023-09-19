qemu-system-x86_64 ^
        -smp cores=4,threads=1 -m 4096 ^
        -vga virtio ^
        -machine q35,smm=on ^
        -global driver=cfi.pflash01,property=secure,value=on ^
        -drive if=pflash,format=raw,unit=0,file="OVMF_CODE_4M.ms.fd",readonly=on ^
        -drive if=pflash,format=raw,unit=1,file="OVMF_VARS_4M.ms.fd" ^
        -drive file=file.iso,format=raw