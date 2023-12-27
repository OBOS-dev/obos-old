# drivers/generic/test/CMakeLists.txt

# Copyright (c) 2023 Omar Berrow

add_executable(initrdDriver "main.cpp"
	"${CMAKE_SOURCE_DIR}/src/oboskrnl/driverInterface/x86_64/call_interface.cpp" 
	"${CMAKE_SOURCE_DIR}/src/oboskrnl/arch/x86_64/syscall/syscall_interface.c" "connection_handler.cpp" "parse.cpp")

target_compile_options(initrdDriver
	PRIVATE "-ffreestanding"
	PRIVATE "-nostdlib"
	PRIVATE "-fPIC"
	PRIVATE "${DEBUG_SYMBOLS_OPT}"
)
set_property (TARGET initrdDriver PROPERTY CXX_STANDARD 20)

set_target_properties(initrdDriver PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

target_compile_definitions(initrdDriver PRIVATE OBOS_DRIVER=1)

target_include_directories(initrdDriver PRIVATE "${CMAKE_SOURCE_DIR}/src/oboskrnl")

target_link_options(initrdDriver
	PRIVATE "-ffreestanding"
	PRIVATE "-nostdlib"
	PRIVATE "-pie"
)

add_dependencies(initrdDriver oboskrnl)