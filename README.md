# OBOS
## Goals
- [x] A module loader/interface
- [x] An initrd driver
- [x] A VFS
- [ ] PS2 Keyboard driver
- [x] AHCI Driver
- [ ] NVME Driver.
- [x] A FAT driver.
- [ ] Ext2/3/4 Driver.
- [ ] POSIX Compatibility Layer
- [ ] Porting mlibc.
- [ ] A terminal.
- [ ] Support for USB.
- [ ] Support for ACPI power management.
- [ ] Porting binutils and GCC to the os, and making a GCC target for the os.
- [ ] Porting CMake and Make.
- [ ] Porting tar.
- [ ] A network stack.
- [ ] An audio stack.
- [ ] A GUI.
- [ ] Porting python.
- [ ] Porting 7zip.
- [ ] Porting a browser and curl.
- [ ] Porting qemu.
## Building
### Prerequisites
- CMake
- Ninja
- Make
- xorriso
- An x86_64-elf-gcc cross compiler if you are building for x86_64.
- WSL2 if you are on windows.
### Build Instructions (x86_64)
1. Open a terminal (or WSL if on windows), and clone the GitHub repository with these commands:
```bash
git clone https://github.com/oberrow/obos.git
cd obos
git clone https://github.com/limine-bootloader/limine.git --branch=v5.2023-20241006.0-binary --depth=1
```
You can ignore any warnings.<br>
2. Configure cmake with this command:
```bash
cmake --toolchain src/scripts-toolchains/x86_64/toolchain.cmake -DCMAKE_BUILD_TYPE=Debug -G Ninja .
```
3. Finally, build the kernel with these commands:
```bash
make -C limine
ninja -j 0
```

The iso image will be stored in out/obos.iso<br>
If you have qemu installed, you can run a virtual machine with the os by running scripts/launch-qemu.sh
