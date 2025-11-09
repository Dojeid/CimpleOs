ASM = nasm
CC = gcc
LD = ld

CFLAGS = -ffreestanding -fno-pie -m32 -g -Ikernel
ASMFLAGS = -felf32

all: CimpleOS.iso

CimpleOS.bin: boot.o kernel.o toolchain/linker.ld
	$(LD) -m elf_i386 -T toolchain/linker.ld -o CimpleOS.bin boot.o kernel.o

boot.o: boot/boot.asm
	$(ASM) $(ASMFLAGS) boot/boot.asm -o boot.o

kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel.o

CimpleOS.iso: CimpleOS.bin iso/boot/grub/grub.cfg
	mkdir -p iso/boot
	cp CimpleOS.bin iso/boot/CimpleOS.bin
	grub-mkrescue -o CimpleOS.iso iso

run: CimpleOS.iso
	virtualbox --startvm "CimpleOS" &

clean:
	rm -f *.o *.bin CimpleOS.iso
	rm -rf iso/boot