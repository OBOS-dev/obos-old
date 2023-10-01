# src/oboskrnl/CMakeLists.txt
#
# Copyright (c) 2023 Omar Berrow

set(oboskrnl_files ${oboskrnl_asmSources} ${oboskrnl_platformSpecific} 
        "boot/kmain.cpp"            "console.cpp"                              "klog.cpp"                           "inline-asm.cpp"
        "klog.cpp"                  "utils/bitfields.cpp"                      "descriptors/idt/pic.cpp"            "external/list/list_iterator.c"
        "utils/memory.cpp"          "memory_manager/paging/liballoc_hooks.cpp" "memory_manager/liballoc/liballoc.cpp" "multitasking/threadHandle.cpp"
        "external/list/list_node.c" "external/list/list.c"                     "multitasking/multitasking.cpp"      "elf/elf.cpp"
        "multitasking/thread.cpp"   "multitasking/mutex/mutex.cpp"             "multitasking/mutex/mutexHandle.cpp" "process/process.cpp"
        "driver_api/interrupts.cpp" "driver_api/syscalls.cpp"                  "syscalls/syscalls.cpp"              "external/amalgamated-dist/Zydis.c"
        "descriptors/gdt/gdt.cpp" "error.cpp")

add_executable(oboskrnl ${oboskrnl_files})
target_compile_options(oboskrnl PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall> PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wextra> PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Werror>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=misleading-indentation> PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-builtin-declaration-mismatch> 
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=unknown-pragmas>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-error=array-bounds>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-volatile>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wno-overloaded-virtual>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti> PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-delete-null-pointer-checks>
                                PRIVATE $<$<COMPILE_LANGUAGE:C>:-fno-delete-null-pointer-checks>
                                # PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Og>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-mgeneral-regs-only>
                                PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${TARGET_COMPILE_OPTIONS}>
                                PRIVATE $<$<COMPILE_LANGUAGE:C>:${TARGET_COMPILE_OPTIONS}>
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

target_compile_definitions(oboskrnl PUBLIC CURSOR_SLEEP_TIME_MS=${CURSOR_SLEEP_TIME_MS}
                                    PUBLIC CONSOLE_NEWLINE_SEQUENCE=\"${CONSOLE_NEWLINE_SEQUENCE}\" 
                                    PUBLIC OBOS_ARCHITECTURE=${OBOS_ARCHITECTURE} 
                                    PUBLIC ZYAN_NO_LIBC=1)

message("Using linker script at " "${CMAKE_SOURCE_DIR}/${LINKER_SCRIPT}")
message("crtbegin.o is at ${CRTBEGIN_DIRECTORY}")
message("crtend.o is at ${CRTBEGIN_DIRECTORY}")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("Debug configuration.")
    target_compile_definitions(oboskrnl PUBLIC _DEBUG=1)
else()
    message(FATAL_ERROR "CMAKE_BUILD_TYPE must be \"Debug\".")
endif()

if (${OBOS_ARCHITECTURE} STREQUAL "x86_64")
    target_compile_definitions(oboskrnl PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:__x86_64__>)
    message ("Building for x86_64")
elseif(${OBOS_ARCHITECTURE} STREQUAL "i686")
    target_compile_definitions(oboskrnl PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:__i686__>)
    message ("Building for i686 (x86)")
else()
    message(FATAL_ERROR "Unknown architecture \"" ${OBOS_ARCHITECTURE} "\" make sure to set it to x86_64 to build for x86_64, or i686 for x86.")
endif()

add_custom_command(TARGET oboskrnl POST_BUILD
                   COMMAND echo "Generating the iso..."
                   COMMAND cp -u out/nvme isodir/obos/initrd/nvme
                   COMMAND objcopy -S isodir/obos/initrd/nvme
                   COMMAND cp -u out/ps2Keyboard isodir/obos/initrd/ps2Keyboard
                   COMMAND objcopy -S isodir/obos/initrd/ps2Keyboard
                   COMMAND cp -u out/testProgram isodir/obos/initrd/testProgram
                   COMMAND objcopy -S isodir/obos/initrd/testProgram
                   COMMAND cp -u out/ustarFilesystem isodir/obos/drivers/filesystems/ustarFilesystem
                   COMMAND objcopy -S isodir/obos/drivers/filesystems/ustarFilesystem
                   COMMAND nm out/oboskrnl --demangle=gnu-v3 -ln | grep -w --ignore-case T > isodir/obos/oboskrnl.map
                   COMMAND cd scripts
                   COMMAND ./make_initrd.sh
                   COMMAND cd ..
                   COMMAND cp -u out/oboskrnl isodir/obos/oboskrnl
                   COMMAND objcopy -S isodir/obos/oboskrnl
                   COMMAND cp -u out/ps2Keyboard isodir/obos/drivers/input/ps2Keyboard
                   COMMAND cp -u out/ustarFilesystem isodir/obos/drivers/filesystems/ustarFilesystem
                   COMMAND grub-mkrescue -o ${OUTPUT_DIR}/obos.iso isodir 2> /dev/zero > /dev/zero
                   COMMAND echo "Finished build!"
                   WORKING_DIRECTORY ${OUTPUT_DIR}/../
                   BYPRODUCTS "${OUTPUT_DIR}/obos.iso")