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

set (oboskrnl_sources "boot/kmain.cpp" "console.cpp" "klog.cpp" "liballoc/liballoc.cpp" 
					  "multitasking/scheduler.cpp" "error.cpp" "multitasking/threadAPI/thrHandle.cpp" "multitasking/process/process.cpp"
 "driverInterface/interface.cpp")

add_executable(oboskrnl ${oboskrnl_platformSpecificSources} ${oboskrnl_sources})

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
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wno-error=array-bounds>
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

target_compile_definitions(oboskrnl PUBLIC OBOS_KERNEL=1)

add_custom_command(TARGET oboskrnl POST_BUILD
                   COMMAND echo "Generating the iso..."
				   COMMAND cp -u out/oboskrnl isodir/obos/oboskrnl
				   COMMAND objcopy -S isodir/obos/oboskrnl
				   COMMAND cp -u out/initrdDriver isodir/obos/initrdDriver
				   COMMAND objcopy -S isodir/obos/initrdDriver
				   COMMAND nm out/oboskrnl --demangle=gnu-v3 -ln | grep -w --ignore-case T > isodir/obos/oboskrnl.map
				   COMMAND cd scripts
				   COMMAND ./make_initrd.sh
				   COMMAND cd ..
				   COMMAND xorriso -as mkisofs -b limine/limine-bios-cd.bin  -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label isodir -o ${OUTPUT_DIR}/obos.iso > /dev/null 2> /dev/null
				   COMMAND ./limine/limine bios-install ${OUTPUT_DIR}/obos.iso > /dev/null 2> /dev/null
                   COMMAND echo "Finished build!"
                   WORKING_DIRECTORY ${OUTPUT_DIR}/../
                   BYPRODUCTS "${OUTPUT_DIR}/obos.iso")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(oboskrnl PUBLIC "OBOS_DEBUG")
	target_compile_options(oboskrnl PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-g -F dwarf>)
else (CMAKE_BUILD_TYPE STREQUAL "Release")
	target_compile_definitions(oboskrnl PUBLIC "OBOS_RELEASE")
endif()