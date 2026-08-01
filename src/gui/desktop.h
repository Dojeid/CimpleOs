#ifndef DESKTOP_H
#define DESKTOP_H

#include <stdint.h>

// Desktop configuration
#define DESKTOP_TOPBAR_HEIGHT 25
#define DESKTOP_TASKBAR_HEIGHT 30

// System-wide Theme Colors Structure
typedef struct {
    uint32_t bg_color;
    uint32_t topbar_color;
    uint32_t taskbar_color;
    uint32_t titlebar_active;
    uint32_t titlebar_inactive;
    uint32_t accent_color;
    uint32_t text_primary;
} gui_theme_t;

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

// System Theme Controls
gui_theme_t* theme_get_current(void);
void desktop_set_theme(int theme_id);

#endif
