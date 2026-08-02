#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include "include/multiboot.h"

#define THEME_DARK  0
#define THEME_LIGHT 1

extern int screen_w, screen_h;
extern uint32_t* video_memory;
extern uint32_t* back_buffer;

// Core init
void graphics_init(struct multiboot_info* mb);
void graphics_set_mode(int width, int height, int bpp);

// Pixel
void put_pixel(int x, int y, uint32_t color);

// Rectangles
void draw_rect(int x, int y, int w, int h, uint32_t color);
void draw_rect_alpha(int x, int y, int w, int h, uint32_t color, uint8_t alpha);

// Rounded rectangles
void draw_rounded_rect(int x, int y, int w, int h, int r, uint32_t color);
void draw_rounded_rect_alpha(int x, int y, int w, int h, int r, uint32_t color, uint8_t alpha);
void draw_rounded_rect_outline(int x, int y, int w, int h, int r, int thickness, uint32_t color);

// Circles
void draw_circle(int cx, int cy, int radius, uint32_t color);
void draw_circle_alpha(int cx, int cy, int radius, uint32_t color, uint8_t alpha);

// Gradients
void draw_linear_gradient(int x, int y, int w, int h, uint32_t color_start, uint32_t color_end, int vertical);
void draw_gradient_rounded_rect(int x, int y, int w, int h, int r, uint32_t c1, uint32_t c2, int vertical);

// Text
void draw_char(int x, int y, char c, uint32_t color);
void draw_string(int x, int y, uint32_t color, const char* str);
void draw_string_scaled(int x, int y, uint32_t color, const char* str, int scale);
void draw_string_shadow(int x, int y, uint32_t color, uint32_t shadow_color, const char* str);

// Effects
void draw_window_shadow(int x, int y, int w, int h);
void draw_box_blur(int x, int y, int w, int h, int radius);

// Buffer
void swap_buffers(void);
void clear_screen(uint32_t color);

// Settings
void graphics_set_theme(int theme);
int  graphics_get_theme(void);
void graphics_set_brightness(int level_percent);
int  graphics_get_brightness(void);
void graphics_set_night_light(int enabled);
int  graphics_get_night_light(void);
int  graphics_get_real_fps(void);

#endif
