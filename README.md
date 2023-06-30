# obos
## Compiling
### Before starting, two requirements must be met:
- You must be on a Linux operating system (if you're on windows, install WSL2 and you can use that as your Linux).
<br>
- You must have an i686-elf cross-compiler. Instructions on how to get can be found here: [GCC Cross-Compiler - OSDev Wiki](https://wiki.osdev.org/GCC_Cross-Compiler).<br>You will need GCC 11.4.0 and binutils 2.40.
<br>
- To generate the operating system's iso, you must have xorriso installed.
<br>
<br>
### Let's get to the build process now.
- First, Clone the repository with this command:
<br>
`git clone https://github.com/oberrow/obos.git`
<br>
- Second, enter the directory that the repository was cloned into.
<br>
`cd obos`
<br>
- Third, configure CMake with this command if you want to compile in debug mode:
<br>
`cmake ./ -G Ninja -DLINKER_SCRIPT=kernel/linker.ld "-DOUTPUT_DIR=./out" -DCMAKE_BUILD_TYPE=Debug`
<br>
or like this to compile in release mode:
<br>
`cmake ./ -G Ninja -DLINKER_SCRIPT=kernel/linker.ld "-DOUTPUT_DIR=./out" -DCMAKE_BUILD_TYPE=Release`
<br>
- Fourth, build everything with this command:
`ninja -j 0 all`
<br>
- Finally, generate an iso with this command:
`./generate_iso.sh`
<br>
This iso will be stored in the release directory.