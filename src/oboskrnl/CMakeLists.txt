# src/oboskrnl/CMakeLists.txt
#
# Copyright (c) 2023 Omar Berrow

set_source_files_properties(SOURCE "crti/src/crti.S" ASM-ATT)
set_source_files_properties(SOURCE "crti/src/crte.S" ASM-ATT)

set(oboskrnl_cxxSources "boot/kmain.cpp"            "console.cpp"                              "klog.cpp"                           "inline-asm.cpp"
                        "klog.cpp"                  "utils/bitfields.cpp"                      "descriptors/idt/pic.cpp"            "external/list/list_iterator.c"
                        "utils/memory.cpp"          "memory_manager/paging/liballoc_hooks.cpp" "memory_manager/liballoc/liballoc.c" "multitasking/threadHandle.cpp"
                        "external/list/list_node.c" "external/list/list.c"                     "multitasking/multitasking.cpp"      "elf/elf.cpp"
                        "multitasking/thread.cpp"   "multitasking/mutex/mutex.cpp"             "multitasking/mutex/mutexHandle.cpp" "process/process.cpp"
                        "driver_api/interrupts.cpp" "driver_api/syscalls.cpp"                  "syscalls/syscalls.cpp"              "external/amalgamated-dist/Zydis.c"
                        "descriptors/gdt/gdt.cpp"
)

# set(oboskrnl_headers "types.h"                            "boot/multiboot.h"                   "console.h"                    "klog.h"
#                      "inline-asm.h"                       "descriptors/gdt/gdt.h"              "descriptors/idt/idt.h"        "memory_manager/physical.h"
#                      "utils/bitfields.h"                  "descriptors/idt/pic.h"              "memory_manager/paging/init.h" "memory_manager/paging/allocate.h" 
#                      "utils/memory.h"                     "memory_manager/liballoc/liballoc.h" "utils/list.h"                 "external/list/list.h"
#                      "multitasking/threadHandle.h"        "multitasking/multitasking.h"        "multitasking/mutex/mutex.h"   "multitasking/mutex/mutexHandle.h"
#                      "new"                                "elf/elf.h"                          "elf/elfStructures.h"          "driver_api/interrupts.h"
#                      "utils/hashTable.h"                  "process/process.h"                  "syscalls/syscalls.h"          "external/amalgamated-dist/Zydis.h"
#                      "boot/boot.h"
# )

# file(GLOB_RECURSE oboskrnl_cxxSources *.cpp *.c)
# file(GLOB_RECURSE oboskrnl_headers *.h)
# file(GLOB_RECURSE oboskrnl_asmSources *.asm)

set(oboskrnl_files ${oboskrnl_asmSources} ${oboskrnl_platformSpecific} ${oboskrnl_cxxSources})

add_executable(oboskrnl ${oboskrnl_files})
target_compile_options(oboskrnl PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall> PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wextra> PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Werror>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=misleading-indentation> PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-builtin-declaration-mismatch> 
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=unknown-pragmas>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=array-bounds>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-overloaded-virtual>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti> PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-g>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-delete-null-pointer-checks>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-mno-red-zone>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-mcmodel=kernel>
                                PRIVATE $<$<COMPILE_LANGUAGE:C>:-fno-delete-null-pointer-checks>
                                PRIVATE $<$<COMPILE_LANGUAGE:C>:-mno-red-zone>
                                PRIVATE $<$<COMPILE_LANGUAGE:C>:-mcmodel=kernel>
)
set_target_properties(oboskrnl PROPERTIES LINK_DEPENDS "${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT}")
target_link_options(oboskrnl 
    PUBLIC "-T" PUBLIC "${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT}"
#    PUBLIC "${CMAKE_SOURCE_DIR}/src/oboskrnl/crti/src/crti.S" 
#    PUBLIC "${CRTBEGIN_DIRECTORY}"
#    PUBLIC "${CRTEND_DIRECTORY}"
#    PUBLIC "${CMAKE_SOURCE_DIR}/src/oboskrnl/crti/src/crtn.S"
    PUBLIC "-Xlinker" PUBLIC "-Map" PUBLIC "${OUTPUT_DIR}/oboskrnl.map"
    PUBLIC "-ffreestanding"
    PUBLIC "-mcmodel=kernel"
    PUBLIC "-nostdlib")
target_link_libraries(oboskrnl PUBLIC ${LIBGCC})
set_target_properties(oboskrnl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
target_include_directories(oboskrnl PUBLIC "${CMAKE_SOURCE_DIR}/src/oboskrnl")

target_compile_definitions(oboskrnl PUBLIC CURSOR_SLEEP_TIME_2_5_MS=${CURSOR_SLEEP_TIME_2_5_MS}
                                    PUBLIC CONSOLE_NEWLINE_SEQUENCE=\"${CONSOLE_NEWLINE_SEQUENCE}\" 
                                    PUBLIC OBOS_ARCHITECTURE=${OBOS_ARCHITECTURE} 
                                    PUBLIC ZYAN_NO_LIBC=1)

message("Using linker script at " "${CMAKE_SOURCE_DIR}/scripts/linker.ld")
message("crtbegin.o is at ${CRTBEGIN_DIRECTORY}")
message("crtend.o is at ${CRTBEGIN_DIRECTORY}")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("Debug configuration.")
    target_compile_definitions(oboskrnl PUBLIC _DEBUG=1)
    target_compile_options(oboskrnl PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-g>)
else()
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must be \"Debug\".")
endif()