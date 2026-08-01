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
    int active_theme_id;
} desktop_t;

// Initialize desktop
void desktop_init();

// Render desktop background & icons
void desktop_render_background();

// Render top bar
void desktop_render_topbar();

// Handle clicks on desktop icons
void desktop_handle_click(int x, int y);

// Get desktop state
desktop_t* desktop_get_state();

// Set background color
void desktop_set_bg_color(uint32_t color);

// Set desktop color theme
void desktop_set_theme(int theme_id);

#endif
