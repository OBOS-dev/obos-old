# CMakeLists.txt
#
# Copyright (c) 2023 Omar Berrow

set(CMAKE_ASM_NASM_OBJECT_FORMAT "elf32")

enable_language(C ASM ASM_NASM)

set(obos_driverapi_cSources "DriverAPI.c")
set(obos_driverapi_asmSources)
set(obos_driverapi_headers "DriverAPI.h")

set(obos_driverapi_files ${obos_driverapi_asmSources} ${obos_driverapi_cSources} ${obos_driverapi_headers})

add_library(obos_driverapi ${obos_driverapi_files})

target_compile_options(obos_driverapi PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall> PRIVATE $<$<COMPILE_LANGUAGE:C>:-Werror>
                                PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-error=misleading-indentation> PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-builtin-declaration-mismatch> 
                                PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wno-error=unknown-pragmas>)
target_link_options(obos_driverapi 
PUBLIC "-nostdlib"
PUBLIC "-ffreestanding")
set_target_properties(obos_driverapi PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
target_include_directories(obos_driverapi PUBLIC "${CMAKE_SOURCE_DIR}/kernel" PUBLIC "${CMAKE_SOURCE_DIR}")

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    message("Debug configuration.")
    target_compile_definitions(obos_driverapi PUBLIC _DEBUG=1 PUBLIC OBOS=1)
    target_compile_options(obos_driverapi PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-g>)
else()
    message("Release configuration.")
    target_compile_definitions(obos_driverapi PUBLIC NDEBUG=1 PUBLIC OBOS=1)
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