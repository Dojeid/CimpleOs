#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include "include/multiboot.h"

#define THEME_DARK  0
#define THEME_LIGHT 1

void graphics_init(struct multiboot_info* mb);
void graphics_set_mode(int width, int height, int bpp);
void put_pixel(int x, int y, uint32_t color);
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha);
void draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color);
void draw_rounded_rect_alpha(int x, int y, int w, int h, int r, uint32_t color, uint8_t alpha);
void draw_linear_gradient(int x, int y, int w, int h, uint32_t color_start, uint32_t color_end, int vertical);
void draw_char(int x, int y, char c, uint32_t color);
void draw_string(int x, int y, uint32_t color, const char *str);
void draw_window_shadow(int x, int y, int w, int h);
void swap_buffers(void);
void clear_screen(uint32_t color);

void graphics_set_theme(int theme);
int  graphics_get_theme(void);
void graphics_set_brightness(int level_percent);
int  graphics_get_brightness(void);
void graphics_set_night_light(int enabled);
int  graphics_get_night_light(void);

extern int screen_w, screen_h;
extern uint32_t* video_memory;

#endif
