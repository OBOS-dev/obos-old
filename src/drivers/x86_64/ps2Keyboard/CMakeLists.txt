# drivers/x86_64/ps2KeyboardDriver/CMakeLists.txt

# Copyright (c) 2023-2024 Omar Berrow

add_executable (ps2KeyboardDriver "dmain.cpp" "scancodes.cpp" "../../generic/common/new.cpp")

target_compile_options(ps2KeyboardDriver
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-stack-protector -fno-stack-check -fno-lto>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-nostdlib>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-exceptions>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-ffreestanding>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wall>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wextra>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fPIE>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${TARGET_DRIVER_COMPILE_OPTIONS_CPP}>
	PRIVATE $<$<COMPILE_LANGUAGE:C>:${TARGET_DRIVER_COMPILE_OPTIONS_C}>
	PRIVATE "${DEBUG_SYMBOLS_OPT}"
)
set_property (TARGET ps2KeyboardDriver PROPERTY CXX_STANDARD 20)

set_target_properties(ps2KeyboardDriver PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

target_compile_definitions(ps2KeyboardDriver PRIVATE OBOS_DRIVER=1)

target_include_directories(ps2KeyboardDriver PRIVATE "${CMAKE_SOURCE_DIR}/src/oboskrnl")

target_link_options(ps2KeyboardDriver
	PRIVATE "-ffreestanding"
	PRIVATE "-nostdlib"
	PRIVATE "-pie"
)

add_dependencies(ps2KeyboardDriver oboskrnl)