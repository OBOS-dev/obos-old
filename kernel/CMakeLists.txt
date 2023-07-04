set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf32")

enable_language(C ASM ASM_NASM)

set_property (SOURCE kernel_bootstrap-i686.S PROPERTY LANGUAGE ASM)
set_property (SOURCE gdtUpdate.asm PROPERTY LANGUAGE ASM_NASM)

set(oboskrnl_sources "kernel_bootstrap-i686.S" "gdtUpdate.asm" "paging.asm"               "virtual8086.asm"
                     "kmain.c"                 "acpi.c"        "paging.c"                 "terminal.c"
                     "inline-asm.c"            "klog.c"        "kserial.c"                "interrupts.c"
                     "gdt.c"                   "keyboard.c"    "hashTable.c"              "liballoc/liballoc_1_1.c"
                     "bitfields.c"             "multitasking.c"
                     "inline-asm.h"            "paging.h"      "klog.h"                   "kserial.h"
                     "types.h"                 "gdt.h"         "interrupts.h"             "keyboard.h" 
                     "hashTable.h"             "multiboot.h"   "liballoc/liballoc_1_1.h"  "bitfields.h"
                     "multitasking.h")

add_executable(oboskrnl ${oboskrnl_sources})

target_compile_options(oboskrnl PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall> PRIVATE $<$<COMPILE_LANGUAGE:C>:-Werror>
                                PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-error=misleading-indentation> PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-builtin-declaration-mismatch> 
                                PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-error=unknown-pragmas>)
set_target_properties(oboskrnl PROPERTIES LINK_DEPENDS "${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT}")
target_link_options(oboskrnl PUBLIC "-T" PUBLIC "${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT}" PUBLIC "-nostdlib")
# target_link_libraries(oboskrnl PUBLIC "${CMAKE_SOURCE_DIR}/kernel/kernel_bootstrap.o")
set_target_properties(oboskrnl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("Debug configuration.")
else()
    message("Release configuration.")
endif()

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR "i686")

set(CMAKE_C_COMPILER "i686-elf-gcc")
set(CMAKE_ASM_COMPILER "i686-elf-gcc")
set(CMAKE_ASM-ATT_COMPILER "i686-elf-gcc")
set(CMAKE_ASM_NASM_COMPILER "nasm")
set(CMAKE_C_COMPILER_WORKS true)
set(CMAKE_CXX_COMPILER_WORKS true)
set(CMAKE_ASM-ATT_COMPILER_WORKS true)
set(CMAKE_ASM_COMPILER_WORKS true)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)