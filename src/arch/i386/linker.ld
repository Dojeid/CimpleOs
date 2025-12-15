ENTRY(start)

SECTIONS
{
    /* The kernel starts at 1MB */
    . = 1M;

    /* Put the Multiboot header first so GRUB finds it! */
    .text ALIGN(4K) :
    {
        *(.multiboot)
        *(.text)
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata)
    }

    .data ALIGN(4K) :
    {
        *(.data)
    }

    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
    }
}