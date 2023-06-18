enable_language(ASM-ATT)

add_executable(oboskrnl "kmain.c" "acpi.c" "kalloc.c" "terminal.c" 
                        "inline-asm.c" "klog.c" "kserial.c"
                        "inline-asm.h" "kalloc.h" "klog.h" "kserial.h"
                        "types.h")

target_compile_options(oboskrnl PUBLIC "-Wall" PUBLIC "-Werror"
                                PUBLIC "-Wno-error=misleading-indentation" PUBLIC "-Wno-builtin-declaration-mismatch")
target_link_options(oboskrnl PUBLIC "-nostdlib" PUBLIC "${CMAKE_SOURCE_DIR}/int/kernel_bootstrap.o")

set_target_properties(oboskrnl PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT})

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("Debug configuration.")
else()
    message("Release configuration.")
endif()

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR "i386")

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_C_COMPILER "i686-elf-gcc")
set(CMAKE_ASM-ATT_COMPILER "i686-elf-as")

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
set(CMAKE_ASM-ATT_COMPILER_WORKS 1)