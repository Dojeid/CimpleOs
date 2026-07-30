cmake_minimum_required(VERSION 3.16)

# ─────────────────────────────────────────────────────────────────────────────
# Cross-Compiler Auto-Detection
#
# Falkon-OS uses Multiboot 1 (32-bit protected mode entry) then transitions
# to 64-bit long mode. We need the x86_64-elf-gcc cross-compiler which:
#   - Produces genuine ELF64 objects (not Windows PE)
#   - Supports -m64 target properly
#   - Links with proper ELF linker (GNU ld with ELF emulation)
#
# Auto-download is handled by: python build.py -setup
# ─────────────────────────────────────────────────────────────────────────────

set(_CROSS_GCC_CANDIDATES
    "${CMAKE_SOURCE_DIR}/tools/cross/bin/x86_64-elf-gcc.exe"
    "${CMAKE_SOURCE_DIR}/tools/cross/bin/x86_64-elf-gcc"
    "${CMAKE_SOURCE_DIR}/tools/cross/x86_64-elf/bin/gcc.exe"
    "${CMAKE_SOURCE_DIR}/tools/cross/x86_64-elf/bin/gcc"
)

set(FALKON_CROSS_GCC "")
foreach(_cand IN LISTS _CROSS_GCC_CANDIDATES)
    if(EXISTS "${_cand}")
        set(FALKON_CROSS_GCC "${_cand}")
        break()
    endif()
endforeach()

if(FALKON_CROSS_GCC STREQUAL "")
    find_program(FALKON_CROSS_GCC_PATH NAMES "x86_64-elf-gcc" "x86_64-elf-gcc.exe")
    if(FALKON_CROSS_GCC_PATH)
        set(FALKON_CROSS_GCC "${FALKON_CROSS_GCC_PATH}")
    endif()
endif()

if(NOT FALKON_CROSS_GCC STREQUAL "")
    message(STATUS "[FalkonOS] ELF cross-compiler found: ${FALKON_CROSS_GCC}")
    set(CMAKE_C_COMPILER "${FALKON_CROSS_GCC}" CACHE FILEPATH "C compiler" FORCE)
    # Try to find matching ld, ar, objcopy
    string(REPLACE "gcc" "ld"      _CC_LD      "${FALKON_CROSS_GCC}")
    string(REPLACE "gcc" "ar"      _CC_AR      "${FALKON_CROSS_GCC}")
    string(REPLACE "gcc" "objcopy" _CC_OBJCOPY "${FALKON_CROSS_GCC}")
    if(EXISTS "${_CC_LD}")
        set(CMAKE_LINKER "${_CC_LD}" CACHE FILEPATH "Linker" FORCE)
    endif()
    if(EXISTS "${_CC_AR}")
        set(CMAKE_AR "${_CC_AR}" CACHE FILEPATH "Archiver" FORCE)
    endif()
    set(CROSS_COMPILER_FOUND TRUE)
else()
    message(WARNING "[FalkonOS] x86_64-elf cross-compiler NOT found!")
    message(WARNING "  -> Run: python build.py -setup  (auto-downloads cross-compiler)")
    message(WARNING "  -> Falling back to system GCC (may not produce bootable binary on Windows)")
    set(CROSS_COMPILER_FOUND FALSE)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Project
# ─────────────────────────────────────────────────────────────────────────────

# Disable CMake's ABI detection (we're bare-metal freestanding)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(FalkonOS LANGUAGES C ASM_NASM)

# Output binary directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ─────────────────────────────────────────────────────────────────────────────
# Compiler Flags
# ─────────────────────────────────────────────────────────────────────────────

# Kernel C flags: 64-bit freestanding target
set(CMAKE_C_FLAGS
    "-ffreestanding -m64 -mcmodel=kernel"
    "-mno-red-zone -mno-mmx -mno-sse -mno-sse2"
    "-fno-pic -fno-pie"
    "-fno-asynchronous-unwind-tables -fno-exceptions -fno-unwind-tables"
    "-I${CMAKE_SOURCE_DIR}/src")
string(REPLACE ";" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

# NASM: ELF64 objects for the 64-bit kernel
# (boot.asm uses [bits 32] for the Multiboot entry and [bits 64] for long mode)
set(CMAKE_ASM_NASM_FLAGS "-felf64")

# Build-type optimisation
set(CMAKE_C_FLAGS_DEBUG   "-O0 -g -DDEBUG -DDEV_MODE")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG -DRELEASE_MODE")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Linker Flags — produce ELF64 bare-metal binary with custom linker script
# ─────────────────────────────────────────────────────────────────────────────

set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/src/arch/x86_64/linker.ld")

if(CROSS_COMPILER_FOUND)
    # Cross-compiler: native ELF linker works fine
    set(CMAKE_EXE_LINKER_FLAGS
        "-nostdlib -nostartfiles -static -Wl,-T,${LINKER_SCRIPT}")
else()
    # MinGW fallback: force ELF output format explicitly
    set(CMAKE_EXE_LINKER_FLAGS
        "-nostdlib -nostartfiles -static -Wl,-T,${LINKER_SCRIPT} -Wl,--oformat,elf64-x86-64")
endif()

set(CMAKE_C_STANDARD_LIBRARIES "")

# Explicit link command (avoids CMake injecting unwanted library paths)
set(CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET>")

# ─────────────────────────────────────────────────────────────────────────────
# Include directories
# ─────────────────────────────────────────────────────────────────────────────

include_directories(src)

# ─────────────────────────────────────────────────────────────────────────────
# Sources
# ─────────────────────────────────────────────────────────────────────────────

file(GLOB_RECURSE ASM_SOURCES "src/arch/x86_64/*.asm")
file(GLOB_RECURSE C_SOURCES   "src/*.c")

# ─────────────────────────────────────────────────────────────────────────────
# Kernel binary target
# ─────────────────────────────────────────────────────────────────────────────

add_executable(FalkonOS.bin ${ASM_SOURCES} ${C_SOURCES})

set_target_properties(FalkonOS.bin PROPERTIES
    SUFFIX       ""
    LINK_DEPENDS "${LINKER_SCRIPT}"
    OUTPUT_NAME  "FalkonOS.bin"
    ENABLE_EXPORTS FALSE
)

# ─────────────────────────────────────────────────────────────────────────────
# Convenience targets
# ─────────────────────────────────────────────────────────────────────────────

add_custom_target(run_qemu
    COMMAND python "${CMAKE_SOURCE_DIR}/build.py" -run qemu
    DEPENDS FalkonOS.bin
    COMMENT "Launching Falkon-OS in QEMU"
)

add_custom_target(run_vbox
    COMMAND python "${CMAKE_SOURCE_DIR}/build.py" -run vbox
    DEPENDS FalkonOS.bin
    COMMENT "Launching Falkon-OS in VirtualBox"
)
