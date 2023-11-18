# CMakeLists.txt

# Copyright (c) 2023 Omar Berrow

cmake_minimum_required(VERSION 3.5)

project(obos C CXX ASM_NASM)

if (NOT DEFINED OUTPUT_DIR)
	set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/out")
endif()
if (NOT DEFINED GENERATE_BUILD_FILES_FOR_UNIX)
	set(GENERATE_BUILD_FILES_FOR_UNIX 1)
endif()
if (NOT DEFINED E9_HACK)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(E9_HACK "1")
	else()
		set(E9_HACK "0")
	endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT GENERATE_BUILD_FILES_FOR_UNIX)
	set(DEBUG_SYMBOLS_OPT -g)
else()
	set(DEBUG_SYMBOLS_OPT)
endif()

add_subdirectory("src/oboskrnl")
add_subdirectory("src/drivers/generic/initrd")