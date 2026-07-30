cmake_minimum_required(VERSION 3.16)
project(FalkonOS LANGUAGES C ASM_NASM)

# Output directory settings
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Set 64-bit Freestanding Kernel Base Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -m64 -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -I${CMAKE_SOURCE_DIR}/src")
set(CMAKE_ASM_NASM_FLAGS "-felf64")

# Set Build Type Specific Flags (-O optimization levels & preprocessor definitions)
set(CMAKE_C_FLAGS_DEBUG "-O0 -g -DDEBUG -DDEV_MODE")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG -DRELEASE_MODE")

# Default to Debug (dev) mode if CMAKE_BUILD_TYPE is omitted
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build: Debug or Release" FORCE)
endif()

# Set Linker Script and Flags
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/src/arch/x86_64/linker.ld")
set(CMAKE_EXE_LINKER_FLAGS "-n -nostdlib -z max-page-size=0x1000 -T ${LINKER_SCRIPT}")

# Include directory
include_directories(src)

# Source auto-discovery
file(GLOB_RECURSE ASM_SOURCES "src/arch/x86_64/*.asm")
file(GLOB_RECURSE C_SOURCES "src/*.c")

# Define Kernel Executable Target
add_executable(FalkonOS.bin ${ASM_SOURCES} ${C_SOURCES})

# Target properties to prevent standard library linking
set_target_properties(FalkonOS.bin PROPERTIES
    LINK_DEPENDS ${LINKER_SCRIPT}
    OUTPUT_NAME "FalkonOS.bin"
)

# Custom command to generate FalkonOS.iso using GRUB
set(ISODIR "${CMAKE_BINARY_DIR}/isodir")
set(ISO_PATH "${CMAKE_BINARY_DIR}/FalkonOS.iso")
set(GRUB_CFG "${CMAKE_SOURCE_DIR}/src/arch/x86_64/boot/grub.cfg")

add_custom_target(FalkonOS.iso ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ISODIR}/boot/grub"
    COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:FalkonOS.bin>" "${ISODIR}/boot/FalkonOS.bin"
    COMMAND ${CMAKE_COMMAND} -E copy "${GRUB_CFG}" "${ISODIR}/boot/grub/grub.cfg"
    COMMAND grub-mkrescue -o "${ISO_PATH}" "${ISODIR}"
    COMMAND ${CMAKE_COMMAND} -E echo "[✓] Build Complete: ${ISO_PATH}"
    DEPENDS FalkonOS.bin
    COMMENT "Generating bootable FalkonOS.iso with GRUB"
)

# Custom target: Run in VirtualBox
add_custom_target(run
    COMMAND python3 "${CMAKE_SOURCE_DIR}/build.py" --vbox
    DEPENDS FalkonOS.iso
    COMMENT "Launching Falkon-OS in VirtualBox"
)

# Custom target: Run in QEMU
add_custom_target(qemu
    COMMAND qemu-system-x86_64 -cdrom "${ISO_PATH}" -m 512M -vga std
    DEPENDS FalkonOS.iso
    COMMENT "Launching Falkon-OS in QEMU"
)
