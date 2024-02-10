# drivers/generic/acpi/CMakeLists.txt

# Copyright (c) 2023-2024 Omar Berrow

include("uACPI/uacpi.cmake")

uacpi_add_sources()

add_library(acpiDriver STATIC "main.cpp" "../common/new.cpp" ${UACPI_SOURCES} "impl.cpp" "stdlib_impl.cpp")

target_compile_options(acpiDriver
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-stack-protector -fno-stack-check -fno-lto>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-nostdlib>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-exceptions>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-ffreestanding>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wall>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wextra>
	PRIVATE $<$<COMPILE_LANGUAGE:C>:${TARGET_COMPILE_OPTIONS_C}>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${TARGET_COMPILE_OPTIONS_CPP}>
	PRIVATE "${DEBUG_SYMBOLS_OPT}"
)
set_property (TARGET acpiDriver PROPERTY CXX_STANDARD 20)

set_target_properties(acpiDriver PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

target_compile_definitions(acpiDriver PRIVATE OBOS_DRIVER=1 PRIVATE UACPI_OVERRIDE_ARCH_HELPERS=1)

target_include_directories(acpiDriver PRIVATE "${CMAKE_SOURCE_DIR}/src/oboskrnl")
target_include_directories(acpiDriver PRIVATE "${CMAKE_SOURCE_DIR}/limine")
target_include_directories(acpiDriver PUBLIC "${CMAKE_SOURCE_DIR}/src/drivers/generic/acpi/makeshift_include")
target_include_directories(acpiDriver PUBLIC ${UACPI_INCLUDES})

target_link_options(acpiDriver
	PRIVATE "-ffreestanding"
	PRIVATE "-nostdlib"
)