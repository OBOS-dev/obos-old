cmake_minimum_required (VERSION 3.8)

set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf32")

project ("obos" C CXX ASM-ATT ASM_NASM)

# Include the kernel.
add_subdirectory ("src/oboskrnl")
