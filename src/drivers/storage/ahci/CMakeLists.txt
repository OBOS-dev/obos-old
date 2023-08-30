# src/drivers/filesystems/ustar/CMakeLists.txt
#
# Copyright (c) 2023 Omar Berrow

# The ustar filesystem driver.

add_executable (ahci "${CMAKE_SOURCE_DIR}/src/oboskrnl/driver_api/syscall_interface.asm" "${CMAKE_SOURCE_DIR}/src/oboskrnl/driver_api/syscall_interface.h" 
"dmain.c" "enumerate_pci.c" "enumerate_pci.h" "ahciStructures.h")

target_compile_options(ahci PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall> PRIVATE $<$<COMPILE_LANGUAGE:C>:-Werror> PRIVATE $<$<COMPILE_LANGUAGE:C>:-std=gnu17>)

target_link_options(ahci
    PUBLIC "-Xlinker" PUBLIC "-Map" PUBLIC "${OUTPUT_DIR}/ahci.map"
    PUBLIC "-ffreestanding"
    PUBLIC "-T" PUBLIC "${CMAKE_SOURCE_DIR}/src/drivers/storage/ahci/linker.ld"
    PUBLIC "-nostdlib")

set_target_properties(ahci PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
target_compile_definitions(ahci PUBLIC OBOS=1)

target_include_directories(ahci PUBLIC "${CMAKE_SOURCE_DIR}/src/oboskrnl")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("Debug configuration.")
    target_compile_definitions(ahci PUBLIC _DEBUG=1)
    target_compile_options(ahci PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-g>)
else()
    message("Release configuration.")
    target_compile_definitions(ahci PUBLIC NDEBUG=1)
endif()

if (${OBOS_ARCHITECTURE} STREQUAL "x86_64")
    target_compile_options(ahci PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-d__x86_64__>)
elseif(${OBOS_ARCHITECTURE} STREQUAL "i686")
    target_compile_options(ahci PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-d__i686__>)
else()
endif()