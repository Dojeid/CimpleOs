# Falkon-OS CMake Toolchain File
# Forces use of the MSYS2 native GCC (ELF-targeting) instead of MinGW (PE-targeting)
# This ensures the kernel is linked as a proper ELF binary.

cmake_minimum_required(VERSION 3.16)

# Detect MSYS2 base path
set(_MSYS_BASH_CANDIDATES
    "D:/msys64/usr/bin/bash.exe"
    "D:/Apps/msys64/usr/bin/bash.exe"
    "C:/msys64/usr/bin/bash.exe"
    "C:/Program Files/msys64/usr/bin/bash.exe"
)

set(MSYS2_ROOT "")
foreach(_cand IN LISTS _MSYS_BASH_CANDIDATES)
    if(EXISTS "${_cand}")
        get_filename_component(MSYS2_ROOT "${_cand}" DIRECTORY)  # usr/bin
        get_filename_component(MSYS2_ROOT "${MSYS2_ROOT}" DIRECTORY) # usr
        get_filename_component(MSYS2_ROOT "${MSYS2_ROOT}" DIRECTORY) # root
        break()
    endif()
endforeach()

if(MSYS2_ROOT STREQUAL "")
    # Fallback: try to find from PATH
    find_program(MSYS_GCC_EXE "x86_64-pc-msys-gcc")
    if(NOT MSYS_GCC_EXE)
        message(WARNING "MSYS2 not found. Using default system GCC (may produce PE format).")
        return()
    endif()
else()
    set(MSYS_GCC_EXE  "${MSYS2_ROOT}/usr/bin/gcc.exe")
    set(MSYS_GXX_EXE  "${MSYS2_ROOT}/usr/bin/g++.exe")
    set(MSYS_LD_EXE   "${MSYS2_ROOT}/usr/bin/ld.exe")
    set(MSYS_AR_EXE   "${MSYS2_ROOT}/usr/bin/ar.exe")
    set(MSYS_AS_EXE   "${MSYS2_ROOT}/usr/bin/as.exe")
    set(MSYS_NASM_EXE "${MSYS2_ROOT}/ucrt64/bin/nasm.exe")
endif()

# Validate
if(NOT EXISTS "${MSYS_GCC_EXE}")
    message(WARNING "MSYS2 GCC not found at '${MSYS_GCC_EXE}'. Installing: pacman -S msys/gcc msys/binutils")
    return()
endif()

message(STATUS "[Toolchain] Using MSYS2 ELF GCC: ${MSYS_GCC_EXE}")

# Set the system and target
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compilers
set(CMAKE_C_COMPILER   "${MSYS_GCC_EXE}"  CACHE FILEPATH "C compiler" FORCE)
set(CMAKE_CXX_COMPILER "${MSYS_GXX_EXE}" CACHE FILEPATH "C++ compiler" FORCE)

# Assembler (use NASM from ucrt64 if available, else from MSYS2)
if(EXISTS "${MSYS_NASM_EXE}")
    set(CMAKE_ASM_NASM_COMPILER "${MSYS_NASM_EXE}" CACHE FILEPATH "NASM assembler" FORCE)
else()
    find_program(CMAKE_ASM_NASM_COMPILER nasm)
endif()

# Linker tools
if(EXISTS "${MSYS_LD_EXE}")
    set(CMAKE_LINKER "${MSYS_LD_EXE}" CACHE FILEPATH "Linker" FORCE)
endif()
if(EXISTS "${MSYS_AR_EXE}")
    set(CMAKE_AR "${MSYS_AR_EXE}" CACHE FILEPATH "Archiver" FORCE)
endif()

# Don't try to run executables for cross-compilation detection
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Skip compiler ABI detection (we're bare metal)
set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
