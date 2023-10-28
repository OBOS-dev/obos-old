# drivers/generic/test/CMakeLists.txt

# Copyright (c) 2023 Omar Berrow

add_executable(testDriver "main.cpp")

target_compile_options(testDriver
	PUBLIC "-ffreestanding"
	PUBLIC "-nostdlib"
)

set_target_properties(testDriver PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

target_include_directories(testDriver PUBLIC "${CMAKE_SOURCE_DIR}/src/oboskrnl")

target_link_options(testDriver
	PUBLIC "-ffreestanding"
	PUBLIC "-nostdlib"
)