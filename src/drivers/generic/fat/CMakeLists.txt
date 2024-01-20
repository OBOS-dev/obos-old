# drivers/generic/fat/CMakeLists.txt

# Copyright (c) 2023-2024 Omar Berrow

add_executable(fatDriver "main.cpp" "../common/new.cpp" "cache.cpp" "interface.cpp")

target_compile_options(fatDriver
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-stack-protector -fno-stack-check -fno-lto>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-use-cxa-atexit>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-rtti>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-nostdlib>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-exceptions>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-ffreestanding>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wall>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wextra>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fPIE>
	PRIVATE "${DEBUG_SYMBOLS_OPT}"
)
set_property (TARGET fatDriver PROPERTY CXX_STANDARD 20)

set_target_properties(fatDriver PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

target_compile_definitions(fatDriver PRIVATE OBOS_DRIVER=1)

target_include_directories(fatDriver PRIVATE "${CMAKE_SOURCE_DIR}/src/oboskrnl")

target_link_options(fatDriver
	PRIVATE "-ffreestanding"
	PRIVATE "-nostdlib"
	PRIVATE "-pie"
)

add_dependencies(fatDriver oboskrnl)