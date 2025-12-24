#ifndef DESKTOP_H
#define DESKTOP_H

#include <stdint.h>

// Desktop configuration
#define DESKTOP_TOPBAR_HEIGHT 25
#define DESKTOP_TASKBAR_HEIGHT 30

// Desktop state
typedef struct {
    uint32_t bg_color;
    uint32_t topbar_color;
    int show_wallpaper;
} desktop_t;

// Initialize desktop
void desktop_init();

// Render desktop background
void desktop_render_background();

// Render top bar
void desktop_render_topbar();

// Get desktop state
desktop_t* desktop_get_state();

// Set background color
void desktop_set_bg_color(uint32_t color);

#endif
