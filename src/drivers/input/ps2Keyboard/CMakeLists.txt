# src/drivers/input/ps2Keyboard/CMakeLists.txt
#
# Copyright (c) 2023 Omar Berrow

# The ps/2 keyboard driver.

add_executable (ps2Keyboard "dmain.cpp" "${CMAKE_SOURCE_DIR}/src/oboskrnl/driver_api/syscall_interface.asm" "${CMAKE_SOURCE_DIR}/src/oboskrnl/driver_api/syscall_interface.h")

target_compile_options(ps2Keyboard 
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall> 
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Werror> 
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20>
    PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-mno-red-zone>
)

target_link_options(ps2Keyboard
    PUBLIC "-ffreestanding"
    PUBLIC "-nostdlib")

set_target_properties(ps2Keyboard PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
target_compile_definitions(ps2Keyboard PUBLIC OBOS=1)

target_include_directories(ps2Keyboard PUBLIC "${CMAKE_SOURCE_DIR}/src/oboskrnl")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("Debug configuration.")
    target_compile_definitions(ps2Keyboard PUBLIC _DEBUG=1)
    target_compile_options(ps2Keyboard PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-g>)
else()
    message("Release configuration.")
    target_compile_definitions(ps2Keyboard PUBLIC NDEBUG=1)
endif()

if (${OBOS_ARCHITECTURE} STREQUAL "x86_64")
    target_compile_options(ps2Keyboard PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-d__x86_64__>)
elseif(${OBOS_ARCHITECTURE} STREQUAL "i686")
    target_compile_options(ps2Keyboard PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-d__i686__>)
else()
endif()