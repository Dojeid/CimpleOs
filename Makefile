# Makefile for Falkon-OS - Traditional GNU Make Build Driver
# Cross-compilation target for 64-bit bare-metal kernel (x86_64-elf)

# Project directories
ROOT_DIR = .
SRC_DIR = $(ROOT_DIR)/src
BUILD_DIR = $(ROOT_DIR)/build
OUT_DIR = $(ROOT_DIR)/out
TOOLS_DIR = $(ROOT_DIR)/tools

# Toolchain configuration (assumes cross-compiler is installed)
CC = x86_64-elf-gcc
CXX = x86_64-elf-g++
AS = nasm
LD = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy

# Compiler flags for bare-metal x86_64 kernel
CFLAGS = -ffreestanding -m64 -mcmodel=small -I$(SRC_DIR) -I$(SRC_DIR)/include
CFLAGS += -Wall -Wextra -Wno-unused-parameter
CFLAGS += -mno-red-zone -mno-mmx -mno-sse -mno-sse2
CFLAGS += -fno-pic -fno-pie -fno-stack-protector
CFLAGS += -fno-asynchronous-unwind-tables -fno-exceptions -fno-unwind-tables

CXXFLAGS = $(CFLAGS) -std=c++17 -fno-rtti
ASFLAGS = -felf64

# Linker script
LINKER_SCRIPT = $(SRC_DIR)/arch/x86_64/linker.ld

# Source files
ASM_SOURCES = $(wildcard $(SRC_DIR)/arch/x86_64/*.asm)
ASM_SOURCES := $(filter-out $(SRC_DIR)/arch/x86_64/bootsector.asm, $(ASM_SOURCES))
C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
CPP_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)

# Object files
ASM_OBJS = $(patsubst $(SRC_DIR)/arch/x86_64/%.asm,$(BUILD_DIR)/%.o,$(ASM_SOURCES))
C_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
CPP_OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SOURCES))

# Targets
TARGET_BIN = $(OUT_DIR)/FalkonOS.bin
TARGET_FLAT = $(BUILD_DIR)/FalkonOS_flat.bin
TARGET_IMG = $(OUT_DIR)/FalkonOS.img
TARGET_ISO = $(OUT_DIR)/FalkonOS.iso

# Default profile flags
PROFILE ?= dev

ifeq ($(PROFILE),dev)
    CFLAGS += -O0 -g -DDEBUG -DDEV_MODE
    CXXFLAGS += -O0 -g -DDEBUG -DDEV_MODE
else ifeq ($(PROFILE),release)
    CFLAGS += -O2 -DNDEBUG -DRELEASE_MODE
    CXXFLAGS += -O2 -DNDEBUG -DRELEASE_MODE
else ifeq ($(PROFILE),debug)
    CFLAGS += -O0 -g3 -DDEBUG_MODE
    CXXFLAGS += -O0 -g3 -DDEBUG_MODE
else ifeq ($(PROFILE),asan)
    CFLAGS += -O1 -fsanitize=address -g
    CXXFLAGS += -O1 -fsanitize=address -g
else ifeq ($(PROFILE),ubsan)
    CFLAGS += -O1 -fsanitize=undefined -g
    CXXFLAGS += -O1 -fsanitize=undefined -g
else ifeq ($(PROFILE),tsan)
    CFLAGS += -O1 -fsanitize=thread -g
    CXXFLAGS += -O1 -fsanitize=thread -g
endif

# Default target
all: build

# Build the kernel
build: $(TARGET_BIN)

# Link kernel executable
$(TARGET_BIN): $(ASM_OBJS) $(C_OBJS) $(CPP_OBJS) | $(OUT_DIR) $(BUILD_DIR)
	$(LD) -nostdlib -static -T $(LINKER_SCRIPT) \
		$(ASM_OBJS) $(C_OBJS) $(CPP_OBJS) \
		-o $(BUILD_DIR)/FalkonOS.bin
	$(OBJCOPY) -O binary --set-section-flags .bss=alloc,load,contents \
		$(BUILD_DIR)/FalkonOS.bin $(TARGET_FLAT)

# Compile C source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Assemble NASM files
$(BUILD_DIR)/%.o: $(SRC_DIR)/arch/x86_64/%.asm
	$(AS) $(ASFLAGS) $< -o $@

# Ensure build directories exist
$(OUT_DIR) $(BUILD_DIR):
	mkdir -p $@

# Clean object files only
clean:
	rm -rf $(BUILD_DIR)/*.o

# Clean all build artifacts
distclean:
	rm -rf $(BUILD_DIR) $(OUT_DIR)

.PHONY: all build clean distclean
