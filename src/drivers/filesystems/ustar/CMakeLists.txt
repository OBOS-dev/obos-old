# src/drivers/filesystems/ustar/CMakeLists.txt
#
# Copyright (c) 2023 Omar Berrow

# The ustar filesystem driver.

add_executable (ustarFilesystem "dmain.c" "${CMAKE_SOURCE_DIR}/src/oboskrnl/driver_api/syscall_interface.asm" "${CMAKE_SOURCE_DIR}/src/oboskrnl/driver_api/syscall_interface.h")

target_compile_options(ustarFilesystem PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall> PRIVATE $<$<COMPILE_LANGUAGE:C>:-Werror> PRIVATE $<$<COMPILE_LANGUAGE:C>:-std=gnu17>)

target_link_options(ustarFilesystem
    PUBLIC "-Xlinker" PUBLIC "-Map" PUBLIC "${OUTPUT_DIR}/ustarFilesystem.map"
    PUBLIC "-ffreestanding"
    PUBLIC "-T" PUBLIC "${CMAKE_SOURCE_DIR}/src/drivers/storage/nvme/linker.ld"
    PUBLIC "-nostdlib")

set_target_properties(ustarFilesystem PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
target_compile_definitions(ustarFilesystem PUBLIC OBOS=1)

target_include_directories(ustarFilesystem PUBLIC "${CMAKE_SOURCE_DIR}/src/oboskrnl")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("Debug configuration.")
    target_compile_definitions(ustarFilesystem PUBLIC _DEBUG=1)
    target_compile_options(ustarFilesystem PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-g>)
else()
    message("Release configuration.")
    target_compile_definitions(ustarFilesystem PUBLIC NDEBUG=1)
endif()

if (${OBOS_ARCHITECTURE} STREQUAL "x86_64")
    target_compile_options(ustarFilesystem PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-d__x86_64__>)
elseif(${OBOS_ARCHITECTURE} STREQUAL "i686")
    target_compile_options(ustarFilesystem PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-d__i686__>)
else()
endif()