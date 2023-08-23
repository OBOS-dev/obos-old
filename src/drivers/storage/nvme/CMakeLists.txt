# src/drivers/storage/nvme/CMakeLists.txt
#
# Copyright (c) 2023 Omar Berrow

# The ustar filesystem driver.

add_executable (nvme "${CMAKE_SOURCE_DIR}/src/oboskrnl/driver_api/syscall_interface.asm" "${CMAKE_SOURCE_DIR}/src/oboskrnl/driver_api/syscall_interface.h" 
"dmain.c" "enumerate_pci.c" "enumerate_pci.h" "nvme.h")

target_compile_options(nvme PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall> PRIVATE $<$<COMPILE_LANGUAGE:C>:-Werror>)

target_link_options(nvme
    PUBLIC "-Xlinker" PUBLIC "-Map" PUBLIC "${OUTPUT_DIR}/nvme.map"
    PUBLIC "-ffreestanding"
#    PUBLIC "-T" PUBLIC "${CMAKE_SOURCE_DIR}/src/drivers/storage/nvme/linker.ld"
    PUBLIC "-nostdlib")

set_target_properties(nvme PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
target_compile_definitions(nvme PUBLIC OBOS=1)

target_include_directories(nvme PUBLIC "${CMAKE_SOURCE_DIR}/src/oboskrnl")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("Debug configuration.")
    target_compile_definitions(nvme PUBLIC _DEBUG=1)
    target_compile_options(nvme PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-g>)
else()
    message("Release configuration.")
    target_compile_definitions(nvme PUBLIC NDEBUG=1)
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