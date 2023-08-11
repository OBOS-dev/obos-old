# src/drivers/input/ps2Keyboard/
#
# Copyright (c) 2023 Omar Berrow

# The ps/2 keyboard driver.

add_executable (ps2Keyboard "dmain.c")

target_compile_options(ps2Keyboard PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall> PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Werror>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=misleading-indentation> PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-builtin-declaration-mismatch> 
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=unknown-pragmas>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti> PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)

target_link_options(ps2Keyboard 
    PUBLIC "${CMAKE_SOURCE_DIR}/src/oboskrnl/crti/src/crti.S" 
    PUBLIC "${CRTBEGIN_DIRECTORY}"
    PUBLIC "${CRTEND_DIRECTORY}"
    PUBLIC "${CMAKE_SOURCE_DIR}/src/oboskrnl/crti/src/crtn.S"
    PUBLIC "-Xlinker" PUBLIC "-Map" PUBLIC "${OUTPUT_DIR}/ps2Keyboard.map"
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