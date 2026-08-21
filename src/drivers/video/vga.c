#include "vga.h"

// VGA Text Mode (80x25) - 64-bit safe
uint16_t* vga_buffer = (uint16_t*)(uintptr_t)0xB8000;
const int VGA_WIDTH = 80;
const int VGA_HEIGHT = 25;

int vga_row = 0;
int vga_col = 0;

uint16_t vga_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

void vga_clear() {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', 0x0F);
        }
    }
    vga_row = 0;
    vga_col = 0;
}

void vga_putchar_color(char c, uint8_t color) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else {
        vga_buffer[vga_row * VGA_WIDTH + vga_col] = vga_entry(c, color);
        vga_col++;
        if (vga_col >= VGA_WIDTH) {
            vga_col = 0;
            vga_row++;
        }
    }
    
    if (vga_row >= VGA_HEIGHT) {
        for (int y = 1; y < VGA_HEIGHT; y++) {
            for (int x = 0; x < VGA_WIDTH; x++) {
                vga_buffer[(y - 1) * VGA_WIDTH + x] = vga_buffer[y * VGA_WIDTH + x];
            }
        }
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', 0x0F);
        }
        vga_row = VGA_HEIGHT - 1;
    }
}

void vga_putchar(char c) {
    vga_putchar_color(c, 0x0F);
}

void vga_print_color(const char* str, uint8_t color) {
    while (*str) {
        vga_putchar_color(*str, color);
        str++;
    }
}

void vga_print(const char* str) {
    vga_print_color(str, 0x0F);
}

void vga_print_boot_ok(const char* message) {
    vga_print_color("[  ", 0x07);              // Light Gray
    vga_print_color("OK", 0x0A);               // Bright Green
    vga_print_color("  ] ", 0x07);             // Light Gray
    vga_print_color(message, 0x0F);            // Bright White
    vga_print_color("\n", 0x0F);
}
