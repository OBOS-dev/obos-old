enable_language(C ASM-ATT)

set(oboskrnl_sources "kernel_bootstrap-i686.S"
                     "kmain.c" "acpi.c" "kalloc.c" "terminal.c"
                     "inline-asm.c" "klog.c" "kserial.c"
                     "inline-asm.h" "kalloc.h" "klog.h" "kserial.h"
                     "types.h")

add_executable(oboskrnl ${oboskrnl_sources})

target_compile_options(oboskrnl PUBLIC "-Wall" PUBLIC "-Werror"
                                PUBLIC "-Wno-error=misleading-indentation" PUBLIC "-Wno-builtin-declaration-mismatch")
set_target_properties(oboskrnl PROPERTIES LINK_DEPENDS "${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT}")
target_link_options(oboskrnl PUBLIC "-T" PUBLIC "${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT}" PUBLIC "-nostdlib")
target_link_libraries(oboskrnl PUBLIC "${CMAKE_SOURCE_DIR}/kernel/kernel_bootstrap.o")
set_target_properties(oboskrnl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("Debug configuration.")
else()
    message("Release configuration.")
endif()

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR "i386")

set(CMAKE_C_COMPILER "/home/omar/opt/cross/bin/i686-elf-gcc")
set(CMAKE_ASM-ATT_COMPILER "/home/omar/opt/cross/bin/i686-elf-as")
set(CMAKE_C_COMPILER_WORKS true)
set(CMAKE_CXX_COMPILER_WORKS true)
set(CMAKE_ASM-ATT_COMPILER_WORKS true)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)