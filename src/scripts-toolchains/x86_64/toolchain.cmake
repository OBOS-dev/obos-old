set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR "x86-64")

set(CMAKE_C_COMPILER "x86_64-elf-gcc")
set(CMAKE_CXX_COMPILER "x86_64-elf-g++")
set(CMAKE_ASM-ATT_COMPILER "x86_64-elf-gcc")
set(CMAKE_ASM_NASM_COMPILER "nasm")
set(CMAKE_C_COMPILER_WORKS true)
set(CMAKE_CXX_COMPILER_WORKS true)
set(CMAKE_ASM-ATT_COMPILER_WORKS true)
set(CMAKE_ASM_NAME_COMPILER_WORKS true)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf64")

execute_process(COMMAND x86_64-elf-gcc --print-file-name=libgcc.a OUTPUT_VARIABLE LIBGCC)

string(STRIP "${LIBGCC}" LIBGCC)

set(TARGET_COMPILE_OPTIONS_CPP -mno-red-zone  -mcmodel=kernel -fno-omit-frame-pointer)
set(TARGET_COMPILE_OPTIONS_C ${TARGET_COMPILE_OPTIONS_CPP})
set(TARGET_LINKER_OPTIONS -mcmodel=kernel)

set (LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/src/scripts-toolchains/x86_64/linker.ld")
set (oboskrnl_platformSpecificSources 
	"boot/x86_64/kmain_arch.cpp" "x86_64-utils/memory_manipulation.asm" "x86_64-utils/asm.asm" "arch/x86_64/gdt.cpp"
	"arch/x86_64/gdt.asm" "arch/x86_64/idt.cpp" "arch/x86_64/idt.asm" "arch/x86_64/int_handlers.asm"
	"arch/x86_64/trace.cpp" "arch/x86_64/irq/irq.cpp" "arch/x86_64/exception_handlers.cpp" "arch/x86_64/memory_manager/physical/allocate.cpp"
	"arch/x86_64/memory_manager/virtual/initialize.cpp" "arch/x86_64/memory_manager/virtual/allocate.cpp" "arch/x86_64/irq/timer.cpp" "multitasking/x86_64/taskSwitchImpl.asm"
	"multitasking/x86_64/setupFrameInfo.cpp" "multitasking/x86_64/scheduler_bootstrapper.cpp" "multitasking/process/x86_64/procInfo.cpp" "multitasking/process/x86_64/loader/elf.cpp"
	"driverInterface/x86_64/load.cpp" "driverInterface/x86_64/call.cpp" "driverInterface/x86_64/driver_call.asm" "multitasking/x86_64/calibrate_timer.asm"
	"arch/x86_64/syscall/register.cpp" "arch/x86_64/syscall/memory_syscalls.cpp" "arch/x86_64/syscall/verify_pars.cpp" "arch/x86_64/syscall/console_syscalls.cpp"
	"arch/x86_64/syscall/thread_syscalls.cpp" "arch/x86_64/sse.asm" 
)