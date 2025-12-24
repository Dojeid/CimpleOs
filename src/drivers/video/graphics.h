#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include "include/multiboot.h"

void graphics_init(struct multiboot_info* mb);
void put_pixel(int x, int y, uint32_t color);
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_char(int x, int y, char c, uint32_t color);
void draw_string(int x, int y, uint32_t color, const char *str);
void swap_buffers();
void clear_screen(uint32_t color);

extern int screen_w, screen_h;
extern uint32_t* video_memory;

#endif
