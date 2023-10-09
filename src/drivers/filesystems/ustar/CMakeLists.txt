# src/drivers/filesystems/ustar/CMakeLists.txt
#
# Copyright (c) 2023 Omar Berrow

# The ustar filesystem driver.

add_executable (ustarFilesystem "dmain.cpp" "connection.cpp" 
                   "${CMAKE_SOURCE_DIR}/src/oboskrnl/driver_api/syscall_interface.asm" "${CMAKE_SOURCE_DIR}/src/oboskrnl/syscalls/syscall_interface.asm")

target_compile_options(ustarFilesystem 
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall> 
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wextra> 
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Werror>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-mgeneral-regs-only>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-mno-red-zone>)

target_link_options(ustarFilesystem
    PUBLIC "-ffreestanding"
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