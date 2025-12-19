#ifndef COMMON_H
#define COMMON_H

// PIC Constants
#define PIC_MASTER_CMD  0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_CMD   0xA0
#define PIC_SLAVE_DATA  0xA1
#define PIC_EOI         0x20

// IDT Flags
#define IDT_FLAG_PRESENT 0x80
#define IDT_FLAG_RING0   0x00
#define IDT_FLAG_INT_GATE 0x0E
#define IDT_GATE_FLAGS   (IDT_FLAG_PRESENT | IDT_FLAG_RING0 | IDT_FLAG_INT_GATE) // 0x8E

// GDT Selectors
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10

// Colors
#define COLOR_BLACK   0x000000
#define COLOR_WHITE   0xFFFFFF
#define COLOR_RED     0xFF0000
#define COLOR_GREEN   0x00FF00
#define COLOR_BLUE    0x0000FF
#define COLOR_YELLOW  0xFFFF00
#define COLOR_GRAY    0x111111
#define COLOR_LGRAY   0xAAAAAA
#define COLOR_TITLEBAR 0x333333
#define COLOR_WINBG   0xCCCCCC

// Memory
#define PAGE_SIZE     4096
#define BLOCK_SIZE    4096

#endif
