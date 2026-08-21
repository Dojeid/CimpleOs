cmake_minimum_required(VERSION 3.16)

# MinGW's PE/COFF linker cannot honour the ELF linker script (section layout is
# sparse and the flat image lacks the Multiboot header offset, .rdata and .bss).
# On Windows we therefore cross-compile to ELF using clang + lld. On Linux the
# native gcc toolchain already produces proper ELF objects, so it is left alone.
# NOTE: must be set before project() so it takes effect on a fresh configure.
if(WIN32)
    set(CMAKE_C_COMPILER   "clang")
    set(CMAKE_CXX_COMPILER "clang++")
    set(CMAKE_C_COMPILER_TARGET   "x86_64-elf")
    set(CMAKE_CXX_COMPILER_TARGET "x86_64-elf")
    set(CMAKE_C_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
    set(CMAKE_CXX_STANDARD_LIBRARIES "" CACHE STRING "" FORCE)
    set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "" CACHE STRING "" FORCE)
    set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "" CACHE STRING "" FORCE)
endif()

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(FalkonOS LANGUAGES C CXX ASM_NASM)

# Compiler Flags: Freestanding 64-bit Kernel (-mcmodel=small for host GCC compatibility)
set(CMAKE_C_FLAGS
    "-ffreestanding -m64 -mcmodel=small"
    "-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-shadow -Wno-format-security -Wno-misleading-indentation"
    "-mno-red-zone -mno-mmx -mno-sse -mno-sse2"
    "-fno-pic -fno-pie -fno-stack-protector"
    "-fno-asynchronous-unwind-tables -fno-exceptions -fno-unwind-tables"
    "-I${CMAKE_SOURCE_DIR}/src"
    "-I${CMAKE_SOURCE_DIR}/src/include"
)
string(REPLACE ";" " " CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

# NASM Assembler Flags: ELF64 objects
set(CMAKE_ASM_NASM_FLAGS "-felf64")

# Optimization profiles
set(CMAKE_C_FLAGS_DEBUG     "-O0 -g -DDEBUG -DDEV_MODE")
set(CMAKE_C_FLAGS_RELEASE   "-O2 -DNDEBUG -DRELEASE_MODE")
set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -g -DDEBUG -DDEV_MODE")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG -DRELEASE_MODE")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# Sanitizer profiles
if(DEFINED ENV{ASAN})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif()

if(DEFINED ENV{UBSAN})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
endif()

if(DEFINED ENV{TSAN})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
endif()

# Linker script & bare-metal flags
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/src/arch/x86_64/linker.ld")
set(CMAKE_C_STANDARD_LIBRARIES "")

if(WIN32)
    # Direct lld invocation for both C and C++
    set(CMAKE_C_LINK_EXECUTABLE
        "ld.lld -m elf_x86_64 -nostdlib -static -T ${LINKER_SCRIPT} <OBJECTS> -o <TARGET>"
    )
    set(CMAKE_CXX_LINK_EXECUTABLE
        "ld.lld -m elf_x86_64 -nostdlib -static -T ${LINKER_SCRIPT} <OBJECTS> -o <TARGET>"
    )
else()
    set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -nostartfiles -static -no-pie -Wl,-T,${LINKER_SCRIPT}")
endif()

# C++ flags: same as C but with -fno-rtti (no RTTI in freestanding kernel)
set(CMAKE_CXX_FLAGS
    "-ffreestanding -m64 -mcmodel=small"
    "-Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-shadow -Wno-format-security -Wno-misleading-indentation"
    "-mno-red-zone -mno-mmx -mno-sse -mno-sse2"
    "-fno-pic -fno-pie -fno-stack-protector"
    "-fno-asynchronous-unwind-tables -fno-exceptions -fno-unwind-tables"
    "-fno-rtti"
    "-std=c++17"
    "-I${CMAKE_SOURCE_DIR}/src"
    "-I${CMAKE_SOURCE_DIR}/src/include"
)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Source Discovery
include_directories(src src/include)

# Subproject: musl-1.2.6 C Standard Library
add_subdirectory(musl-1.2.6)

# Subproject: GNU Bash Software Shell
add_subdirectory(bash)


file(GLOB_RECURSE ASM_SOURCES "src/arch/x86_64/*.asm")
list(FILTER ASM_SOURCES EXCLUDE REGEX ".*/bootsector\\.asm$")
file(GLOB_RECURSE C_SOURCES   "src/*.c")
file(GLOB_RECURSE CXX_SOURCES "src/*.cpp")

# Kernel Executable Target
add_executable(FalkonOS.bin ${ASM_SOURCES} ${C_SOURCES} ${CXX_SOURCES} $<TARGET_OBJECTS:bash>)

set_target_properties(FalkonOS.bin PROPERTIES
    SUFFIX       ""
    LINK_DEPENDS "${LINKER_SCRIPT}"
    OUTPUT_NAME  "FalkonOS.bin"
    ENABLE_EXPORTS FALSE
)

target_link_libraries(FalkonOS.bin PRIVATE musl)

find_program(OBJCOPY_BIN NAMES objcopy llvm-objcopy REQUIRED)

add_custom_command(TARGET FalkonOS.bin POST_BUILD
    COMMAND ${OBJCOPY_BIN} -O binary --set-section-flags .bss=alloc,load,contents $<TARGET_FILE:FalkonOS.bin> ${CMAKE_BINARY_DIR}/FalkonOS_flat.bin
    COMMENT "[FalkonOS] Generated flat raw kernel binary (FalkonOS_flat.bin) including zero-filled .bss"
)
