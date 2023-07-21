# CMakeLists.txt
#
# Copyright (c) 2023 Omar Berrow

set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf32")

enable_language(C ASM ASM_NASM)

set(fat_driver_cSources "fat.c")

set(fat_driver_headers)
set(fat_driver_asmSources)
set(fat_driver_files ${fat_driver_asmSources} ${fat_driver_cSources} ${fat_driver_headers})

add_executable(obos_fat ${fat_driver_files})

target_compile_options(obos_fat PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall> PRIVATE $<$<COMPILE_LANGUAGE:C>:-Werror>
                                PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-error=misleading-indentation> PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-builtin-declaration-mismatch> 
                                PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-error=unknown-pragmas>)
target_link_options(obos_fat 
    PRIVATE "-T" PUBLIC "${CMAKE_SOURCE_DIR}/fatDriver/linker.ld" 
    PRIVATE "-Xlinker" PRIVATE "-e" PRIVATE "OBOS_DriverEntry" 
    PRIVATE "-ffreestanding")
set_target_properties(obos_fat PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
target_include_directories(obos_fat PUBLIC "${CMAKE_SOURCE_DIR}/kernel" PUBLIC "${CMAKE_SOURCE_DIR}")
target_link_libraries(obos_fat PUBLIC obos_driverapi)
# set_target_properties(obos_fat PROPERTIES LINK_DEPENDS "${CMAKE_SOURCE_DIR}/fatDriver/linker.ld")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("Debug configuration.")
    target_compile_definitions(obos_fat PUBLIC _DEBUG=1 PUBLIC OBOS=1)
    target_compile_options(obos_fat PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-g>)
else()
    message("Release configuration.")
    target_compile_definitions(obos_fat PUBLIC NDEBUG=1 PUBLIC OBOS=1)
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