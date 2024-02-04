# oboskrnl/CMakeLists.txt

# Copyright (c) 2023-2024 Omar Berrow

set (oboskrnl_sources "boot/kmain.cpp" "console.cpp" "klog.cpp" "allocators/liballoc.cpp" 
					  "multitasking/scheduler.cpp" "error.cpp" "multitasking/threadAPI/thrHandle.cpp" "multitasking/process/process.cpp"
				      "vfs/mount/mount.cpp" "vfs/fileManip/fileHandle.cpp" "multitasking/locks/mutex.cpp" "allocators/vmm/vmm.cpp"
					  "driverInterface/register.cpp" "vfs/fileManip/directoryIterator.cpp" "vfs/devManip/driveHandle.cpp" "boot/cfg.cpp"
					  "utils/string.cpp" "vfs/devManip/driveIterator.cpp" "allocators/slab.cpp")

add_executable(oboskrnl ${oboskrnl_platformSpecificSources} ${oboskrnl_sources})

set_target_properties(oboskrnl PROPERTIES LINK_DEPENDS "${LINKER_SCRIPT}")
set_target_properties(oboskrnl PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set_target_properties(oboskrnl PROPERTIES ENABLE_EXPORTS 1)
set_target_properties(oboskrnl PROPERTIES CXX_STANDARD 20)
set_target_properties(oboskrnl PROPERTIES CXX_STANDARD_REQUIRED 20)

message("Output Directory: ${OUTPUT_DIR}")

target_include_directories(oboskrnl PUBLIC "${CMAKE_SOURCE_DIR}/limine")
target_include_directories(oboskrnl PUBLIC "${CMAKE_SOURCE_DIR}/src/oboskrnl")

target_compile_definitions(oboskrnl PUBLIC E9_HACK=${E9_HACK})

target_compile_options(oboskrnl 
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wall>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wextra>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Werror>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-ffreestanding>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-nostdlib>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wno-missing-field-initializers>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wno-error=implicit-fallthrough>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-Wno-error=array-bounds>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX,C>:-fno-stack-protector -fno-stack-check -fno-lto>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-use-cxa-atexit>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20>
	PRIVATE $<$<COMPILE_LANGUAGE:C>:${TARGET_COMPILE_OPTIONS_C}>
	PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${TARGET_COMPILE_OPTIONS_CPP}>
	PRIVATE ${DEBUG_SYMBOLS_OPT}
)

target_link_options(oboskrnl
	PUBLIC "-T" PUBLIC "${LINKER_SCRIPT}"
	PUBLIC "-nostdlib"
	PUBLIC ${TARGET_LINKER_OPTIONS}
)

target_link_libraries(oboskrnl PRIVATE ${LIBGCC} PRIVATE acpiDriver)

target_compile_definitions(oboskrnl PUBLIC OBOS_KERNEL=1)

if (UNIX OR GENERATE_BUILD_FILES_FOR_UNIX)
	add_custom_target(generate_iso ALL
		COMMAND echo "Generating the iso..."
		COMMAND cp -u ${OUTPUT_DIR}/oboskrnl ${CMAKE_SOURCE_DIR}/isodir/obos/oboskrnl
		COMMAND ${OBJCOPY} -g isodir/obos/oboskrnl
		COMMAND cp -u ${OUTPUT_DIR}/initrdDriver ${CMAKE_SOURCE_DIR}/isodir/obos/initrdDriver
		COMMAND ${OBJCOPY} -g ${CMAKE_SOURCE_DIR}/isodir/obos/initrdDriver
		COMMAND cp -u ${OUTPUT_DIR}/sataDriver ${CMAKE_SOURCE_DIR}/isodir/obos/initrd/sataDriver
		COMMAND ${OBJCOPY} -g ${CMAKE_SOURCE_DIR}/isodir/obos/initrd/sataDriver
		COMMAND cp -u ${OUTPUT_DIR}/mbrDriver ${CMAKE_SOURCE_DIR}/isodir/obos/initrd/mbrDriver
		COMMAND ${OBJCOPY} -g ${CMAKE_SOURCE_DIR}/isodir/obos/initrd/mbrDriver
		COMMAND cp -u ${OUTPUT_DIR}/gptDriver ${CMAKE_SOURCE_DIR}/isodir/obos/initrd/gptDriver
		COMMAND ${OBJCOPY} -g ${CMAKE_SOURCE_DIR}/isodir/obos/initrd/gptDriver
		COMMAND cp -u ${OUTPUT_DIR}/fatDriver ${CMAKE_SOURCE_DIR}/isodir/obos/initrd/fatDriver
		COMMAND ${OBJCOPY} -g ${CMAKE_SOURCE_DIR}/isodir/obos/initrd/fatDriver
		COMMAND cp -u ${CMAKE_SOURCE_DIR}/limine/limine-bios*.sys ${CMAKE_SOURCE_DIR}/isodir/limine/
		COMMAND cp -u ${CMAKE_SOURCE_DIR}/limine/limine-bios*.bin ${CMAKE_SOURCE_DIR}/isodir/limine/
		COMMAND cp -u ${CMAKE_SOURCE_DIR}/limine/limine-uefi-cd.bin ${CMAKE_SOURCE_DIR}/isodir/limine/
		COMMAND ${OBJCOPY} -g ${CMAKE_SOURCE_DIR}/isodir/obos/initrd/fatDriver
		COMMAND cd scripts
		COMMAND ./make_initrd.sh
		COMMAND cd ..
		COMMAND xorriso -as mkisofs -b limine/limine-bios-cd.bin  -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label isodir -o ${OUTPUT_DIR}/obos.iso > /dev/null 2> /dev/null
		COMMAND ./limine/limine bios-install ${OUTPUT_DIR}/obos.iso > /dev/null 2> /dev/null
		COMMAND echo "Finished build!"
		WORKING_DIRECTORY ${OUTPUT_DIR}/../
		BYPRODUCTS "${OUTPUT_DIR}/obos.iso"
		DEPENDS oboskrnl
		DEPENDS fatDriver
		DEPENDS gptDriver
	    DEPENDS mbrDriver
	    DEPENDS sataDriver
	    DEPENDS initrdDriver
	)
else (WIN32)
	string(REPLACE "/" "\\" OUTPUT_DIR ${OUTPUT_DIR})
	string(REPLACE "/" "\\" SOURCE_DIRECTORY ${CMAKE_SOURCE_DIR})
	add_custom_target(generate_iso ALL
        COMMAND @echo off
        COMMAND echo Generating the iso...
		COMMAND copy /Y "${OUTPUT_DIR}\\oboskrnl" "${SOURCE_DIRECTORY}\\isodir\\obos\\oboskrnl" > NUL 2>&1
		COMMAND ${OBJCOPY} -g isodir/obos/oboskrnl > NUL 2>&1
		COMMAND copy /Y "${OUTPUT_DIR}\\initrdDriver" "${SOURCE_DIRECTORY}\\isodir\\obos\\initrdDriver" > NUL 2>&1
		COMMAND ${OBJCOPY} -g isodir/obos/initrdDriver > NUL 2>&1
		COMMAND copy /Y "${OUTPUT_DIR}\\sataDriver" "${SOURCE_DIRECTORY}\\isodir\\obos\\initrd\\sataDriver" > NUL 2>&1
		COMMAND ${OBJCOPY} -g isodir/obos/initrd/sataDriver > NUL 2>&1
		COMMAND copy /Y "${OUTPUT_DIR}\\mbrDriver" "${SOURCE_DIRECTORY}\\isodir\\obos\\initrd\\mbrDriver" > NUL 2>&1
		COMMAND ${OBJCOPY} -g isodir/obos/initrd/mbrDriver > NUL 2>&1
		COMMAND copy /Y "${OUTPUT_DIR}\\gptDriver" "${SOURCE_DIRECTORY}\\isodir\\obos\\initrd\\gptDriver" > NUL 2>&1
		COMMAND ${OBJCOPY} -g isodir/obos/initrd/gptDriver > NUL 2>&1
		COMMAND copy /Y "${OUTPUT_DIR}\\fatDriver" "${SOURCE_DIRECTORY}\\isodir\\obos\\initrd\\fatDriver" > NUL 2>&1
		COMMAND ${OBJCOPY} -g isodir/obos/initrd/fatDriver > NUL 2>&1
		COMMAND copy /Y ${SOURCE_DIRECTORY}\\limine\\limine-bios*.sys ${SOURCE_DIRECTORY}\\isodir\\limine\\ > NUL 2>&1
		COMMAND copy /Y ${SOURCE_DIRECTORY}\\limine\\limine-bios*.bin ${SOURCE_DIRECTORY}\\isodir\\limine\\ > NUL 2>&1
		COMMAND copy /Y ${SOURCE_DIRECTORY}\\limine\\limine-uefi-cd.bin ${SOURCE_DIRECTORY}\\isodir\\limine\\ > NUL 2>&1
		COMMAND cd scripts
		COMMAND make_initrd.bat
		COMMAND cd ..
		COMMAND xorriso -as mkisofs -b limine/limine-bios-cd.bin  -no-emul-boot -boot-load-size 4 -boot-info-table --efi-boot limine/limine-uefi-cd.bin -efi-boot-part --efi-boot-image --protective-msdos-label isodir -o ${OUTPUT_DIR}/obos.iso > NUL 2>&1
		COMMAND limine/limine.exe bios-install ${OUTPUT_DIR}/obos.iso > NUL 2>&1
        COMMAND echo Finished build!
        WORKING_DIRECTORY ${OUTPUT_DIR}/../
        BYPRODUCTS "${OUTPUT_DIR}/obos.iso"
	    DEPENDS oboskrnl
	    DEPENDS initrdDriver
	    DEPENDS sataDriver
	    DEPENDS gptDriver
	    DEPENDS mbrDriver
	    DEPENDS fatDriver
	)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(oboskrnl PUBLIC "OBOS_DEBUG")
	target_compile_options(oboskrnl PRIVATE $<$<COMPILE_LANGUAGE:ASM_NASM>:-g -F dwarf>)
	target_compile_options(oboskrnl PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:-fstack-protector>)
else (CMAKE_BUILD_TYPE STREQUAL "Release")
	target_compile_definitions(oboskrnl PUBLIC "OBOS_RELEASE")
endif()