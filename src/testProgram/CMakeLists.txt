# src/testProgram/CMakeLists.txt
#
# Copyright (c) 2023 Omar Berrow

# The ps/2 keyboard driver.

add_executable (testProgram "main.c" "${CMAKE_SOURCE_DIR}/src/oboskrnl/syscalls/syscall_interface.asm" "${CMAKE_SOURCE_DIR}/src/oboskrnl/syscalls/syscall_interface.h")

target_compile_options(testProgram PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall> PRIVATE $<$<COMPILE_LANGUAGE:C>:-Werror>)

target_link_options(testProgram
    PUBLIC "-Xlinker" PUBLIC "-Map" PUBLIC "${OUTPUT_DIR}/testProgram.map"
    PUBLIC "-ffreestanding"
    PUBLIC "-T" PUBLIC "${CMAKE_SOURCE_DIR}/src/testProgram/linker.ld"
    PUBLIC "-nostdlib")

set_target_properties(testProgram PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
target_compile_definitions(testProgram PUBLIC OBOS=1)

# set(CMAKE_ASM_NASM_LINK_EXECUTABLE "i686-elf-gcc -g -Xlinker -Map ${OUTPUT_DIR}/testProgram.map -ffreestanding -nostdlib <OBJECTS> -o ${OUTPUT_DIR}/testProgram")

target_include_directories(testProgram PUBLIC "${CMAKE_SOURCE_DIR}/src/oboskrnl")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("Debug configuration.")
    target_compile_definitions(testProgram PUBLIC _DEBUG=1)
    target_compile_options(testProgram PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-g>)
else()
    message("Release configuration.")
    target_compile_definitions(testProgram PUBLIC NDEBUG=1)
endif()

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR "i686")

set(CMAKE_C_COMPILER "i686-elf-gcc")
set(CMAKE_CXX_COMPILER "i686-elf-g++")
set(CMAKE_ASM-ATT_COMPILER "i686-elf-gcc")
set(CMAKE_ASM_NASM_COMPILER "nasm")
set(CMAKE_C_COMPILER_WORKS true)
set(CMAKE_CXX_COMPILER_WORKS true)
set(CMAKE_ASM-ATT_COMPILER_WORKS true)
set(CMAKE_ASM_NAME_COMPILER_WORKS true)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)