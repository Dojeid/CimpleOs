#ifndef VGA_H
#define VGA_H

#include <stdint.h>

// VGA Text Mode (80x25)
extern uint16_t* vga_buffer;
extern const int VGA_WIDTH;
extern const int VGA_HEIGHT;

extern int vga_row;
extern int vga_col;

// VGA functions
uint16_t vga_entry(char c, uint8_t color);
void vga_clear();
void vga_putchar(char c);
void vga_print(const char* str);

#endif