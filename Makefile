# Default architecture is i386 (32-bit).
ARCH ?= i386

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
    # 64-bit settings (Future Proofing)
    CFLAGS := -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -c
    ASMFLAGS := -felf64
    LDFLAGS := -n -nostdlib -z max-page-size=0x1000
else
    # 32-bit settings (Current Default)
    CFLAGS := -ffreestanding -m32 -g -c
    ASMFLAGS := -felf32
    LDFLAGS := -m elf_i386
endif

# Common Include Path (So kernel.c can find .h files)
CFLAGS += -I$(SRCDIR)/kernel

# --- SOURCE AUTO-DISCOVERY ---
# 1. Find all C files in src/kernel/ (kernel.c, gdt.c, idt.c)
KERNEL_SRC := $(wildcard $(SRCDIR)/kernel/*.c)
# Convert .c filenames to .o filenames in the build folder
KERNEL_OBJ := $(patsubst $(SRCDIR)/kernel/%.c, $(BUILDDIR)/kernel/%.o, $(KERNEL_SRC))

# 2. Find all Assembly files in src/arch/i386/ (boot.asm, interrupts.asm)
ASM_SRC := $(wildcard $(SRCDIR)/arch/$(ARCH)/*.asm)
# Convert .asm filenames to .o filenames in the build folder
ASM_OBJ := $(patsubst $(SRCDIR)/arch/$(ARCH)/%.asm, $(BUILDDIR)/arch/$(ARCH)/%.o, $(ASM_SRC))

# Helper to locate the Linker Script and GRUB config
LINKER_SCRIPT := $(SRCDIR)/arch/$(ARCH)/linker.ld
GRUB_CFG := $(SRCDIR)/arch/$(ARCH)/grub.cfg

# --- BUILD RULES ---

all: CimpleOS.iso

# 1. Link everything together (Boot + Interrupts + Kernel + GDT + IDT)
CimpleOS.bin: $(ASM_OBJ) $(KERNEL_OBJ)
	@echo "Linking Kernel..."
	$(LD) $(LDFLAGS) -T $(LINKER_SCRIPT) -o $(BUILDDIR)/CimpleOS.bin $(ASM_OBJ) $(KERNEL_OBJ)

# 2. Compile C Kernel Files (Automatically compiles new files like idt.c)
$(BUILDDIR)/kernel/%.o: $(SRCDIR)/kernel/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling C: $<"
	$(CC) $(CFLAGS) $< -o $@

# 3. Compile Assembly Arch Files (Automatically compiles interrupts.asm)
$(BUILDDIR)/arch/$(ARCH)/%.o: $(SRCDIR)/arch/$(ARCH)/%.asm
	@mkdir -p $(dir $@)
	@echo "Assembling: $<"
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

clean:
	rm