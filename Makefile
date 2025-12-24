# CimpleOS 64-bit Makefile - Linux-Style Structure
ARCH ?= x86_64

# Paths
SRCDIR := src
BUILDDIR := build
ISODIR := isodir

# Tools
ASM := nasm
CC := gcc
LD := ld
GRUB := grub-mkrescue

# --- Architecture Specific Flags ---
ifeq ($(ARCH),x86_64)
    # 64-bit settings
    CFLAGS := -ffreestanding -m64 -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -c
    ASMFLAGS := -felf64
    LDFLAGS := -n -nostdlib -z max-page-size=0x1000
else
    # 32-bit settings (fallback)
    CFLAGS := -ffreestanding -m32 -g -c -mno-sse -mno-sse2 -mno-mmx
    ASMFLAGS := -felf32
    LDFLAGS := -m elf_i386
endif

# Include paths for all source directories
CFLAGS += -I$(SRCDIR)

# --- SOURCE AUTO-DISCOVERY (Linux-Style Directories) ---

# 1. Kernel core files
KERNEL_SRC := $(wildcard $(SRCDIR)/kernel/*.c)
KERNEL_OBJ := $(patsubst $(SRCDIR)/kernel/%.c, $(BUILDDIR)/kernel/%.o, $(KERNEL_SRC))

# 2. Memory management files
MM_SRC := $(wildcard $(SRCDIR)/mm/*.c)
MM_OBJ := $(patsubst $(SRCDIR)/mm/%.c, $(BUILDDIR)/mm/%.o, $(MM_SRC))

# 3. Driver files (nested directories)
DRIVER_INPUT_SRC := $(wildcard $(SRCDIR)/drivers/input/*.c)
DRIVER_VIDEO_SRC := $(wildcard $(SRCDIR)/drivers/video/*.c)
DRIVER_BUS_SRC := $(wildcard $(SRCDIR)/drivers/bus/*.c)
DRIVER_INPUT_OBJ := $(patsubst $(SRCDIR)/drivers/input/%.c, $(BUILDDIR)/drivers/input/%.o, $(DRIVER_INPUT_SRC))
DRIVER_VIDEO_OBJ := $(patsubst $(SRCDIR)/drivers/video/%.c, $(BUILDDIR)/drivers/video/%.o, $(DRIVER_VIDEO_SRC))
DRIVER_BUS_OBJ := $(patsubst $(SRCDIR)/drivers/bus/%.c, $(BUILDDIR)/drivers/bus/%.o, $(DRIVER_BUS_SRC))
DRIVERS_OBJ := $(DRIVER_INPUT_OBJ) $(DRIVER_VIDEO_OBJ) $(DRIVER_BUS_OBJ)

# 4. Library files
LIB_SRC := $(wildcard $(SRCDIR)/lib/*.c)
LIB_OBJ := $(patsubst $(SRCDIR)/lib/%.c, $(BUILDDIR)/lib/%.o, $(LIB_SRC))

# 5. GUI files
GUI_SRC := $(wildcard $(SRCDIR)/gui/*.c)
GUI_OBJ := $(patsubst $(SRCDIR)/gui/%.c, $(BUILDDIR)/gui/%.o, $(GUI_SRC))

# 6. Assembly files (boot and kernel asm in arch subdirectories)
ASM_BOOT_SRC := $(wildcard $(SRCDIR)/arch/$(ARCH)/boot/*.asm)
ASM_KERNEL_SRC := $(wildcard $(SRCDIR)/arch/$(ARCH)/kernel/*.asm)
ASM_BOOT_OBJ := $(patsubst $(SRCDIR)/arch/$(ARCH)/boot/%.asm, $(BUILDDIR)/arch/$(ARCH)/boot/%.o, $(ASM_BOOT_SRC))
ASM_KERNEL_OBJ := $(patsubst $(SRCDIR)/arch/$(ARCH)/kernel/%.asm, $(BUILDDIR)/arch/$(ARCH)/kernel/%.o, $(ASM_KERNEL_SRC))
ASM_OBJ := $(ASM_BOOT_OBJ) $(ASM_KERNEL_OBJ)

# Combined object files
ALL_OBJ := $(ASM_OBJ) $(KERNEL_OBJ) $(MM_OBJ) $(DRIVERS_OBJ) $(LIB_OBJ) $(GUI_OBJ)

# Helper to locate the Linker Script and GRUB config
LINKER_SCRIPT := $(SRCDIR)/arch/$(ARCH)/linker.ld
GRUB_CFG := $(SRCDIR)/arch/$(ARCH)/boot/grub.cfg

# --- BUILD RULES ---

all: CimpleOS.iso

# 1. Link everything together
CimpleOS.bin: $(ALL_OBJ)
	@echo "Linking Kernel..."
	$(LD) $(LDFLAGS) -T $(LINKER_SCRIPT) -o $(BUILDDIR)/CimpleOS.bin $(ALL_OBJ)

# 2. Compile C files from each directory
$(BUILDDIR)/kernel/%.o: $(SRCDIR)/kernel/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling kernel: $<"
	$(CC) $(CFLAGS) $< -o $@

$(BUILDDIR)/mm/%.o: $(SRCDIR)/mm/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling mm: $<"
	$(CC) $(CFLAGS) $< -o $@

$(BUILDDIR)/drivers/input/%.o: $(SRCDIR)/drivers/input/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling driver/input: $<"
	$(CC) $(CFLAGS) $< -o $@

$(BUILDDIR)/drivers/video/%.o: $(SRCDIR)/drivers/video/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling driver/video: $<"
	$(CC) $(CFLAGS) $< -o $@

$(BUILDDIR)/drivers/bus/%.o: $(SRCDIR)/drivers/bus/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling driver/bus: $<"
	$(CC) $(CFLAGS) $< -o $@

$(BUILDDIR)/lib/%.o: $(SRCDIR)/lib/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling lib: $<"
	$(CC) $(CFLAGS) $< -o $@

$(BUILDDIR)/gui/%.o: $(SRCDIR)/gui/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling gui: $<"
	$(CC) $(CFLAGS) $< -o $@

# 3. Compile Assembly files
$(BUILDDIR)/arch/$(ARCH)/boot/%.o: $(SRCDIR)/arch/$(ARCH)/boot/%.asm
	@mkdir -p $(dir $@)
	@echo "Assembling boot: $<"
	$(ASM) $(ASMFLAGS) $< -o $@

$(BUILDDIR)/arch/$(ARCH)/kernel/%.o: $(SRCDIR)/arch/$(ARCH)/kernel/%.asm
	@mkdir -p $(dir $@)
	@echo "Assembling kernel asm: $<"
	$(ASM) $(ASMFLAGS) $< -o $@

# 4. Create the ISO
CimpleOS.iso: CimpleOS.bin
	@mkdir -p $(ISODIR)/boot/grub
	cp $(BUILDDIR)/CimpleOS.bin $(ISODIR)/boot/CimpleOS.bin
	cp $(GRUB_CFG) $(ISODIR)/boot/grub/grub.cfg
	@echo "Generating ISO..."
	$(GRUB) -o CimpleOS.iso $(ISODIR)
	@echo "Build Complete: CimpleOS.iso"

# Run in Emulator
run: all
	virtualbox --startvm "CimpleOS" &

# Clean build files
clean:
	@echo "Cleaning build files..."
	rm -rf build
	rm -rf isodir
	rm -f CimpleOS.iso

# Show what files will be compiled (for debugging)
info:
	@echo "=== SOURCE FILES ==="
	@echo "Kernel: $(KERNEL_SRC)"
	@echo "MM: $(MM_SRC)"
	@echo "Drivers Input: $(DRIVER_INPUT_SRC)"
	@echo "Drivers Video: $(DRIVER_VIDEO_SRC)"
	@echo "Drivers Bus: $(DRIVER_BUS_SRC)"
	@echo "Lib: $(LIB_SRC)"
	@echo "GUI: $(GUI_SRC)"
	@echo "ASM Boot: $(ASM_BOOT_SRC)"
	@echo "ASM Kernel: $(ASM_KERNEL_SRC)"
	@echo "=== OBJECT FILES ==="
	@echo "Total objects: $(words $(ALL_OBJ))"

.PHONY: all clean run info