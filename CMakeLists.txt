# CMakeLists.txt
#
# Copyright (c) 2023 Omar Berrow

cmake_minimum_required (VERSION 3.8)

project ("obos" C CXX ASM-ATT ASM_NASM)

# Include all the sources.
add_subdirectory ("src/oboskrnl/")
add_subdirectory("src/drivers/input/ps2Keyboard/")
add_subdirectory("src/drivers/filesystems/ustar/")
add_subdirectory("src/drivers/storage/ahci/")
add_subdirectory("src/drivers/storage/nvme/")
add_subdirectory("src/testProgram")