/*
 * Falkon-OS Linker Script
 * Produces a Multiboot 1-compliant ELF kernel loaded at 1 MB physical address.
 * Ensures .multiboot header and entry symbol 'start' are at 0x00100000 / 0x00100010.
 */

ENTRY(start)

SECTIONS
{
    /* Load at 1 MB — conventional Multiboot 1 load address */
    . = 1M;

    /* Multiboot header & 32-bit entry stub at top of image (0x100000) */
    .boot :
    {
        KEEP(*(.multiboot))
        *(.boot.text)
    }

    /* Mixed 32-bit boot stub + 64-bit long mode kernel code */
    .text :
    {
        *(.text .text.*)
    }

    /* Read-only data (GDT descriptor, string literals, etc.) */
    .rodata ALIGN(4K) :
    {
        *(.rodata .rodata.*)
    }

    /* Initialised data */
    .data ALIGN(4K) :
    {
        *(.data .data.*)
    }

    /* Zero-initialised data (stack + page tables declared in boot.asm .bss) */
    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss .bss.*)
    }
}
