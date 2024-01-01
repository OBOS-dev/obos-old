set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR "x86-64")

find_program(HAS_CROSS_COMPILER "x86_64-elf-g++")
if (NOT HAS_CROSS_COMPILER)
	message(FATAL_ERROR "No x86_64-elf cross compiler in the PATH!")
endif()

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

find_program(HAS_x86_64_elf_objcopy "x86_64-elf-objcopy")
find_program(HAS_x86_64_elf_nm "x86_64-elf-nm")
if (HAS_x86_64_elf_objcopy)
	set(OBJCOPY "x86_64-elf-objcopy")
else()
	set(OBJCOPY "objcopy")
endif()
if (HAS_x86_64_elf_nm)
	set(NM "x86_64-elf-nm")
else()
	set(NM "nm")
endif()
set(TARGET_COMPILE_OPTIONS_CPP -mno-red-zone -fno-omit-frame-pointer -mgeneral-regs-only -mcmodel=kernel)
set(TARGET_COMPILE_OPTIONS_C ${TARGET_COMPILE_OPTIONS_CPP})
set(TARGET_LINKER_OPTIONS -mcmodel=kernel)

set (LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/src/scripts-toolchains/x86_64/linker.ld")
set (oboskrnl_platformSpecificSources 
	"boot/x86_64/kmain_arch.cpp" "x86_64-utils/memory_manipulation.asm" "x86_64-utils/asm.asm" "arch/x86_64/gdt.cpp"
	"arch/x86_64/gdt.asm" "arch/x86_64/idt.cpp" "arch/x86_64/idt.asm" "arch/x86_64/int_handlers.asm" 
	"arch/x86_64/memory_manager/physical/allocate.cpp"  "arch/x86_64/irq/irq.cpp" "arch/x86_64/exception_handlers.cpp" "arch/x86_64/trace.cpp"
	"arch/x86_64/memory_manager/virtual/initialize.cpp" "arch/x86_64/memory_manager/virtual/allocate.cpp" "arch/x86_64/irq/timer.cpp" "multitasking/x86_64/taskSwitchImpl.asm"
	"multitasking/x86_64/setupFrameInfo.cpp" "multitasking/x86_64/scheduler_bootstrapper.cpp" "multitasking/process/x86_64/procInfo.cpp" "multitasking/process/x86_64/loader/elf.cpp"
	"driverInterface/x86_64/load.cpp" "multitasking/x86_64/calibrate_timer.asm" "arch/x86_64/stack_canary.cpp" "arch/x86_64/fpu.asm"
	"arch/x86_64/syscall/register.cpp" "arch/x86_64/syscall/memory_syscalls.cpp" "arch/x86_64/syscall/verify_pars.cpp" "arch/x86_64/syscall/console_syscalls.cpp"
	"arch/x86_64/syscall/thread_syscalls.cpp" "arch/x86_64/sse.asm" "arch/x86_64/smp_start.cpp" "arch/x86_64/smp_trampoline.asm"
	"arch/x86_64/gdbstub/communicate.cpp" "arch/x86_64/gdbstub/stub.cpp" "arch/x86_64/signals.cpp"
)

set (OBOS_ARCHITECTURE "x86_64")