# sample_programs/x86-64/testProgram/CMakeLists.txt

# Copyright (c) 2023-2024 Omar Berrow

add_executable(test_program "entry.asm" "dummy.c")

target_compile_options(test_program
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-stack-protector -fno-stack-check -fno-lto>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-nostdlib>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-exceptions>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-ffreestanding>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wall>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wextra>
	PRIVATE "${DEBUG_SYMBOLS_OPT}"
)
set_property (TARGET test_program PROPERTY CXX_STANDARD 20)

set_target_properties(test_program PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

target_link_options(test_program
	PRIVATE "-ffreestanding"
	PRIVATE "-nostdlib"
)

add_dependencies(test_program oboskrnl)