#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <stdint.h>

#define MAX_WINDOWS 16
#define TITLEBAR_HEIGHT 22
#define WINDOW_BORDER 2

// Window flags
#define WIN_FLAG_VISIBLE    0x01
#define WIN_FLAG_FOCUSED    0x02
#define WIN_FLAG_MINIMIZED  0x04
#define WIN_FLAG_MAXIMIZED  0x08
#define WIN_FLAG_DRAGGING   0x10

// Window structure
typedef struct window {
    int id;
    int x, y;                    // Position
    int width, height;           // Size
    int saved_x, saved_y;        // For restore from maximize
    int saved_width, saved_height;
    char title[64];
    uint32_t flags;
    
    // Render callback
    void (*render_content)(struct window* win);
    void (*on_close)(struct window* win);
    void (*on_minimize)(struct window* win);
    void (*on_maximize)(struct window* win);
    
    // Internal
    int drag_offset_x, drag_offset_y;
    
    // FEATURE 1: User data for custom window state (e.g., terminal instance)
    void* user_data;
} window_t;

// Window manager state
typedef struct {
    window_t windows[MAX_WINDOWS];
    int window_count;
    int focused_window_id;
    int next_window_id;
} window_manager_t;

// Initialize window manager
void wm_init();

// Window creation/destruction
window_t* wm_create_window(int x, int y, int width, int height, const char* title);
void wm_destroy_window(int window_id);
window_t* wm_get_window(int window_id);

// Window operations
void wm_focus_window(int window_id);
void wm_minimize_window(int window_id);
void wm_maximize_window(int window_id);
void wm_restore_window(int window_id);
void wm_move_window(int window_id, int new_x, int new_y);
void wm_resize_window(int window_id, int new_width, int new_height);

// Window queries
int wm_get_window_at(int x, int y);
int wm_is_point_in_titlebar(window_t* win, int x, int y);
int wm_is_point_in_close_button(window_t* win, int x, int y);
int wm_is_point_in_minimize_button(window_t* win, int x, int y);
int wm_is_point_in_maximize_button(window_t* win, int x, int y);

// Rendering
void wm_render_all();
void wm_render_window(window_t* win);

// Events
void wm_handle_mouse_down(int x, int y);
void wm_handle_mouse_up(int x, int y);
void wm_handle_mouse_move(int x, int y);

// Get window manager state
window_manager_t* wm_get_state();

#endif
