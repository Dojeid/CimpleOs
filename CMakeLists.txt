cmake_minimum_required(VERSION 3.16)
project(FalkonOS LANGUAGES C ASM_NASM)

# ─────────────────────────────────────────────────────────────────────────────
# Falkon-OS Kernel CMake Build Configuration
# Target: 64-bit Freestanding Kernel (x86_64)
# ─────────────────────────────────────────────────────────────────────────────

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Compiler Flags: Freestanding 64-bit Kernel (-mcmodel=small for host GCC compatibility)
set(CMAKE_C_FLAGS
    "-ffreestanding -m64 -mcmodel=small"
    "-Wall -Wextra -Wno-unused-parameter"
    "-mno-red-zone -mno-mmx -mno-sse -mno-sse2"
    "-fno-pic -fno-pie"
    "-fno-asynchronous-unwind-tables -fno-exceptions -fno-unwind-tables"
    "-I${CMAKE_SOURCE_DIR}/src"
)
string(REPLACE ";" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

# NASM Assembler Flags: ELF64 objects
set(CMAKE_ASM_NASM_FLAGS "-felf64")

# Optimization profiles
set(CMAKE_C_FLAGS_DEBUG   "-O0 -g -DDEBUG -DDEV_MODE")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG -DRELEASE_MODE")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# Linker script & bare-metal flags
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/src/arch/x86_64/linker.ld")
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -nostartfiles -static -Wl,-T,${LINKER_SCRIPT}")
set(CMAKE_C_STANDARD_LIBRARIES "")

# Explicit link executable command
set(CMAKE_C_LINK_EXECUTABLE
    "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET>"
)

# Source Discovery
include_directories(src)

file(GLOB_RECURSE ASM_SOURCES "src/arch/x86_64/*.asm")
list(FILTER ASM_SOURCES EXCLUDE REGEX ".*/bootsector\\.asm$")
file(GLOB_RECURSE C_SOURCES   "src/*.c")

# Kernel Executable Target
add_executable(FalkonOS.bin ${ASM_SOURCES} ${C_SOURCES})

set_target_properties(FalkonOS.bin PROPERTIES
    SUFFIX       ""
    LINK_DEPENDS "${LINKER_SCRIPT}"
    OUTPUT_NAME  "FalkonOS.bin"
    ENABLE_EXPORTS FALSE
)
