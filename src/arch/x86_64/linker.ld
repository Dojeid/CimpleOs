/*
 * Falkon-OS Linker Script
 * Produces a Multiboot 1-compliant ELF kernel loaded at 1 MB physical address.
 *
 * Memory layout:
 *   0x100000  - .boot    : Multiboot header (must be in first 8 KB)
 *   0x100000+ - .text    : Kernel code (32-bit boot stub + 64-bit long mode code)
 *   ...       - .rodata  : Read-only data (GDT, strings, etc.)
 *   ...       - .data    : Initialised data
 *   ...       - .bss     : Zero-initialised data (stack, page tables)
 */

ENTRY(start)

SECTIONS
{
    /* Load at 1 MB — conventional Multiboot 1 load address */
    . = 1M;

    /* Multiboot header must appear in the first 8 KB of the image */
    .boot :
    {
        KEEP(*(.multiboot))
    }

    /* Mixed 32-bit boot stub + 64-bit long mode kernel code */
    .text ALIGN(4K) :
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
