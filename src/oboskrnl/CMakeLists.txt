# oboskrnl/CMakeLists.txt

# Copyright (c) 2023 Omar Berrow

if (NOT DEFINED OUTPUT_DIR)
	set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/out")
endif()
if (NOT DEFINED E9_HACK)
	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		set(E9_HACK "1")
	else()
		set(E9_HACK "0")
	endif()
endif()

set (oboskrnl_sources "console.cpp" "klog.cpp" "liballoc/liballoc.cpp" "multitasking/scheduler.cpp"
					  "boot/kmain.cpp")

add_executable(oboskrnl ${oboskrnl_sources} ${oboskrnl_platformSpecificSources})

set_target_properties(oboskrnl PROPERTIES LINK_DEPENDS "${LINKER_SCRIPT}")
set_target_properties(oboskrnl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

target_include_directories(oboskrnl PUBLIC "${CMAKE_SOURCE_DIR}/limine")
target_include_directories(oboskrnl PUBLIC "${CMAKE_SOURCE_DIR}/src/oboskrnl")

target_compile_definitions(oboskrnl PUBLIC E9_HACK=${E9_HACK})

target_compile_options(oboskrnl 
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wall>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wextra>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Werror>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-ffreestanding>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-nostdlib>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-mgeneral-regs-only>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-rtti>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-exceptions>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wno-missing-field-initializers>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wno-error=implicit-fallthrough>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-mcmodel=kernel>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-omit-frame-pointer>
)

target_link_options(oboskrnl
	PUBLIC "-T" PUBLIC "${LINKER_SCRIPT}"
	PUBLIC "-mcmodel=kernel"
	PUBLIC "-ffreestanding"
	PUBLIC "-nostdlib"
)

target_link_libraries(oboskrnl PUBLIC ${LIBGCC})

add_custom_command(TARGET oboskrnl POST_BUILD
                   COMMAND echo "Generating the iso..."
				   COMMAND cp -u out/oboskrnl isodir/obos/oboskrnl
				   COMMAND objcopy -S isodir/obos/oboskrnl
				   COMMAND xorriso -as mkisofs -b limine/limine-bios-cd.bin  -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label isodir -o ${OUTPUT_DIR}/obos.iso
				   COMMAND ./limine/limine bios-install ${OUTPUT_DIR}/obos.iso
                   COMMAND echo "Finished build!"
                   WORKING_DIRECTORY ${OUTPUT_DIR}/../
                   BYPRODUCTS "${OUTPUT_DIR}/obos.iso")