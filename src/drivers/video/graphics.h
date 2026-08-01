#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include "include/multiboot.h"

// BUG FIX #2: Extern declarations for screen dimensions (defined in graphics.c)
extern int screen_w, screen_h;
extern uint32_t* video_memory;
extern uint32_t* back_buffer;

void graphics_init(struct multiboot_info* mb);
void put_pixel(int x, int y, uint32_t color);
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_char(int x, int y, char c, uint32_t color);
void draw_string(int x, int y, uint32_t color, const char *str);
void swap_buffers();
void clear_screen(uint32_t color);

void graphics_set_brightness(int percent);
int graphics_get_brightness(void);
void graphics_set_night_light(int enabled);
int graphics_get_night_light(void);
void graphics_set_fps_target(int fps_preset);
int graphics_get_fps_delay_ticks(void);
void graphics_set_resolution(int width, int height);

#endif
