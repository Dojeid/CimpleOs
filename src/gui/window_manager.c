#include "window_manager.h"
#include "drivers/video/graphics.h"
#include "lib/string.h"
#include "gui/desktop.h"

// Forward declaration for error messages
extern void terminal_print(const char*);

// Color definitions
#define COLOR_WINDOW_BG       0x2C2C2C
#define COLOR_TITLEBAR_ACTIVE 0x2C3E50
#define COLOR_TITLEBAR_INACTIVE 0x555555
#define COLOR_BORDER          0x1A1A1A
#define COLOR_CLOSE_BTN       0xE74C3C
#define COLOR_MIN_BTN         0xF39C12
#define COLOR_MAX_BTN         0x27AE60
#define COLOR_BTN_HOVER       0xFFFFFF

static window_manager_t wm;

void wm_init() {
    wm.window_count = 0;
    wm.focused_window_id = -1;
    wm.next_window_id = 1;
    
    for (int i = 0; i < MAX_WINDOWS; i++) {
        wm.windows[i].id = -1;
        wm.windows[i].flags = 0;
    }
}

window_t* wm_create_window(int x, int y, int width, int height, const char* title) {
    // SECURITY FIX: Validate inputs before use
    if (!title) {
        terminal_print("ERROR: Window title is NULL");
        return NULL;
    }
    if (width < 100 || width > 2000) {
        terminal_print("ERROR: Window width out of range (100-2000)");
        return NULL;
    }
    if (height < 80 || height > 1500) {
        terminal_print("ERROR: Window height out of range (80-1500)");
        return NULL;
    }
    
    if (wm.window_count >= MAX_WINDOWS) {
        terminal_print("ERROR: Maximum windows reached (16)");
        return NULL;
    }
    
    // Find free slot
    int slot = -1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (wm.windows[i].id == -1) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        terminal_print("ERROR: No free window slots");
        return NULL;
    }
    
    window_t* win = &wm.windows[slot];
    win->id = wm.next_window_id++;
    
    // SECURITY FIX: Clamp position to screen bounds
    extern int screen_w, screen_h;
    if (x < 0) x = 0;
    if (y < 25) y = 25;
    if (x + width > screen_w) x = screen_w - width;
    if (y + height > screen_h - 30) y = screen_h - 30 - height;
    
    win->x = x;
    win->y = y;
    win->width = width;
    win->height = height;
    win->saved_x = x;
    win->saved_y = y;
    win->saved_width = width;
    win->saved_height = height;
    
    // SECURITY FIX: Safe string copy with bounds check
    strncpy(win->title, title, 63);
    win->title[63] = '\0';
    
    win->flags = WIN_FLAG_VISIBLE;
    win->render_content = NULL;
    win->on_click = NULL;
    win->on_keydown = NULL;
    win->on_close = NULL;
    win->on_minimize = NULL;
    win->on_maximize = NULL;
    win->drag_offset_x = 0;
    win->drag_offset_y = 0;
    win->user_data = NULL;  // FEATURE 1
    
    wm.window_count++;
    wm_focus_window(win->id);
    
    return win;
}

void wm_destroy_window(int window_id) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (wm.windows[i].id == window_id) {
            // Call close callback if set
            if (wm.windows[i].on_close) {
                wm.windows[i].on_close(&wm.windows[i]);
            }
            
            // BUG FIX: Remove taskbar button
            extern void taskbar_remove_button(int);
            taskbar_remove_button(window_id);
            
            wm.windows[i].id = -1;
            wm.windows[i].flags = 0;
            wm.window_count--;
            
            // Focus another window
            if (wm.focused_window_id == window_id) {
                wm.focused_window_id = -1;
                // Find another visible window
                for (int j = 0; j < MAX_WINDOWS; j++) {
                    if (wm.windows[j].id != -1 && 
                        (wm.windows[j].flags & WIN_FLAG_VISIBLE) &&
                        !(wm.windows[j].flags & WIN_FLAG_MINIMIZED)) {
                        wm_focus_window(wm.windows[j].id);
                        break;
                    }
                }
            }
            return;
        }
    }
}

window_t* wm_get_window(int window_id) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (wm.windows[i].id == window_id) {
            return &wm.windows[i];
        }
    }
    return NULL;
}

void wm_focus_window(int window_id) {
    // Unfocus current
    if (wm.focused_window_id != -1) {
        window_t* old_focused = wm_get_window(wm.focused_window_id);
        if (old_focused) {
            old_focused->flags &= ~WIN_FLAG_FOCUSED;
        }
    }
    
    // Focus new
    wm.focused_window_id = window_id;
    window_t* win = wm_get_window(window_id);
    if (win) {
        win->flags |= WIN_FLAG_FOCUSED;
    }
}

void wm_minimize_window(int window_id) {
    window_t* win = wm_get_window(window_id);
    if (!win) return;
    
    win->flags |= WIN_FLAG_MINIMIZED;
    
    if (win->on_minimize) {
        win->on_minimize(win);
    }
    
    // Focus another window
    if (wm.focused_window_id == window_id) {
        wm.focused_window_id = -1;
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (wm.windows[i].id != -1 && 
                wm.windows[i].id != window_id &&
                (wm.windows[i].flags & WIN_FLAG_VISIBLE) &&
                !(wm.windows[i].flags & WIN_FLAG_MINIMIZED)) {
                wm_focus_window(wm.windows[i].id);
                break;
            }
        }
    }
}

void wm_maximize_window(int window_id) {
    window_t* win = wm_get_window(window_id);
    if (!win) return;
    
    extern int screen_w, screen_h;
    
    if (win->flags & WIN_FLAG_MAXIMIZED) {
        // Restore
        win->x = win->saved_x;
        win->y = win->saved_y;
        win->width = win->saved_width;
        win->height = win->saved_height;
        win->flags &= ~WIN_FLAG_MAXIMIZED;
    } else {
        // Save current size/position
        win->saved_x = win->x;
        win->saved_y = win->y;
        win->saved_width = win->width;
        win->saved_height = win->height;
        
        // Maximize (leave room for taskbar)
        win->x = 0;
        win->y = 25;  // Below top bar
        win->width = screen_w;
        win->height = screen_h - 25 - 30;  // Above taskbar
        win->flags |= WIN_FLAG_MAXIMIZED;
    }
    
    if (win->on_maximize) {
        win->on_maximize(win);
    }
}

void wm_restore_window(int window_id) {
    window_t* win = wm_get_window(window_id);
    if (!win) return;
    
    win->flags &= ~WIN_FLAG_MINIMIZED;
    wm_focus_window(window_id);
}

void wm_move_window(int window_id, int new_x, int new_y) {
    window_t* win = wm_get_window(window_id);
    if (!win) return;
    
    win->x = new_x;
    win->y = new_y;
}

void wm_resize_window(int window_id, int new_width, int new_height) {
    window_t* win = wm_get_window(window_id);
    if (!win) return;
    
    if (new_width < 200) new_width = 200;
    if (new_height < 150) new_height = 150;
    
    win->width = new_width;
    win->height = new_height;
}

int wm_get_window_at(int x, int y) {
    // Check from front to back (focused window first)
    if (wm.focused_window_id != -1) {
        window_t* win = wm_get_window(wm.focused_window_id);
        if (win && (win->flags & WIN_FLAG_VISIBLE) && 
            !(win->flags & WIN_FLAG_MINIMIZED)) {
            if (x >= win->x && x < win->x + win->width &&
                y >= win->y && y < win->y + win->height + TITLEBAR_HEIGHT) {
                return win->id;
            }
        }
    }
    
    // Check other windows
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (wm.windows[i].id == -1) continue;
        if (wm.windows[i].id == wm.focused_window_id) continue;
        if (!(wm.windows[i].flags & WIN_FLAG_VISIBLE)) continue;
        if (wm.windows[i].flags & WIN_FLAG_MINIMIZED) continue;
        
        window_t* win = &wm.windows[i];
        if (x >= win->x && x < win->x + win->width &&
            y >= win->y && y < win->y + win->height + TITLEBAR_HEIGHT) {
            return win->id;
        }
    }
    
    return -1;
}

int wm_is_point_in_titlebar(window_t* win, int x, int y) {
    if (!win) return 0;
    return (x >= win->x && x < win->x + win->width &&
            y >= win->y && y < win->y + TITLEBAR_HEIGHT);
}

int wm_is_point_in_close_button(window_t* win, int x, int y) {
    if (!win) return 0;
    int btn_x = win->x + win->width - 20;
    int btn_y = win->y + 4;
    return (x >= btn_x && x < win->x + win->width &&
            y >= btn_y && y < btn_y + 16);
}

int wm_is_point_in_minimize_button(window_t* win, int x, int y) {
    if (!win) return 0;
    int btn_x = win->x + win->width - 56;
    int btn_y = win->y + 4;
    return (x >= btn_x && x < win->x + win->width - 38 &&
            y >= btn_y && y < btn_y + 16);
}

int wm_is_point_in_maximize_button(window_t* win, int x, int y) {
    if (!win) return 0;
    int btn_x = win->x + win->width - 38;
    int btn_y = win->y + 4;
    return (x >= btn_x && x < win->x + win->width - 20 &&
            y >= btn_y && y < btn_y + 16);
}

void wm_render_window(window_t* win) {
    if (!win || !(win->flags & WIN_FLAG_VISIBLE) || 
        (win->flags & WIN_FLAG_MINIMIZED)) {
        return;
    }
    
    int is_focused = (win->flags & WIN_FLAG_FOCUSED);
    gui_theme_t* theme = theme_get_current();
    
    // Draw Window Drop Shadow
    draw_window_shadow(win->x, win->y, win->width, TITLEBAR_HEIGHT + win->height);
    
    // Draw modern title bar based on active theme
    uint32_t titlebar_color = is_focused ? theme->titlebar_active : theme->titlebar_inactive;
    draw_rect(win->x, win->y, win->width, TITLEBAR_HEIGHT, titlebar_color);
    
    // Title text
    draw_string(win->x + 10, win->y + 7, is_focused ? 0xF1F5F9 : 0x94A3B8, win->title);
    
    // Draw Windows 11 Style Window Control Buttons (Min, Max, Close)
    int btn_y = win->y + 4;
    
    // Close button box (Vibrant Red 0xEF4444)
    int close_btn_x = win->x + win->width - 24;
    draw_rect(close_btn_x, btn_y, 20, 16, 0xEF4444);
    draw_string(close_btn_x + 6, btn_y + 4, 0xFFFFFF, "x");
    
    // Maximize button box (Green 0x10B981)
    int max_btn_x = win->x + win->width - 48;
    draw_rect(max_btn_x, btn_y, 20, 16, 0x10B981);
    draw_string(max_btn_x + 6, btn_y + 4, 0xFFFFFF, "+");

    // Minimize button box (Yellow 0xF59E0B)
    int min_btn_x = win->x + win->width - 72;
    draw_rect(min_btn_x, btn_y, 20, 16, 0xF59E0B);
    draw_string(min_btn_x + 6, btn_y + 4, 0xFFFFFF, "-");
    
    // Draw window content area background
    draw_rect(win->x, win->y + TITLEBAR_HEIGHT, win->width, win->height, COLOR_WINDOW_BG);
    
    // Call render content callback if set
    if (win->render_content) {
        win->render_content(win);
    }
    
    // Draw active window glow border based on theme
    uint32_t border_color = is_focused ? theme->accent_color : 0x334155;
    // Top
    draw_rect(win->x, win->y, win->width, 1, border_color);
    // Bottom
    draw_rect(win->x, win->y + TITLEBAR_HEIGHT + win->height - 1, win->width, 1, border_color);
    // Left
    draw_rect(win->x, win->y, 1, TITLEBAR_HEIGHT + win->height, border_color);
    // Right
    draw_rect(win->x + win->width - 1, win->y, 1, TITLEBAR_HEIGHT + win->height, border_color);
}

void wm_render_all() {
    // Render unfocused windows first
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (wm.windows[i].id == -1) continue;
        if (wm.windows[i].id == wm.focused_window_id) continue;
        wm_render_window(&wm.windows[i]);
    }
    
    // Render focused window last (on top)
    if (wm.focused_window_id != -1) {
        window_t* focused = wm_get_window(wm.focused_window_id);
        if (focused) {
            wm_render_window(focused);
        }
    }
}

void wm_handle_mouse_down(int x, int y) {
    int window_id = wm_get_window_at(x, y);
    if (window_id == -1) return;
    
    window_t* win = wm_get_window(window_id);
    if (!win) return;
    
    // Focus this window
    wm_focus_window(window_id);
    
    // Check button clicks
    if (wm_is_point_in_close_button(win, x, y)) {
        wm_destroy_window(window_id);
        return;
    }
    
    if (wm_is_point_in_minimize_button(win, x, y)) {
        wm_minimize_window(window_id);
        return;
    }
    
    if (wm_is_point_in_maximize_button(win, x, y)) {
        wm_maximize_window(window_id);
        return;
    }
    
    // Start dragging if in title bar
    if (wm_is_point_in_titlebar(win, x, y) && !(win->flags & WIN_FLAG_MAXIMIZED)) {
        win->flags |= WIN_FLAG_DRAGGING;
        win->drag_offset_x = x - win->x;
        win->drag_offset_y = y - win->y;
        return;
    }
    
    // Content area click dispatch
    if (win->on_click && y >= win->y + TITLEBAR_HEIGHT) {
        int rel_x = x - win->x;
        int rel_y = y - (win->y + TITLEBAR_HEIGHT);
        win->on_click(win, rel_x, rel_y);
    }
}

void wm_handle_mouse_up(int x, int y) {
    // Stop dragging all windows
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (wm.windows[i].id != -1) {
            wm.windows[i].flags &= ~WIN_FLAG_DRAGGING;
        }
    }
}

void wm_handle_mouse_move(int x, int y) {
    // Handle window dragging
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (wm.windows[i].id == -1) continue;
        if (!(wm.windows[i].flags & WIN_FLAG_DRAGGING)) continue;
        
        window_t* win = &wm.windows[i];
        int new_x = x - win->drag_offset_x;
        int new_y = y - win->drag_offset_y;
        
        // BUG FIX #13: Simplified bounds clamping
        extern int screen_w, screen_h;
        
        // Prevent window from going off-screen
        if (new_x < 0) new_x = 0;
        if (new_y < 25) new_y = 25;  // Below top bar
        
        // Simple right/bottom bounds (window width/height already set by create/resize)
        int max_x = screen_w - win->width;
        int max_y = screen_h - 30 - TITLEBAR_HEIGHT - win->height;
        
        if (new_x > max_x) new_x = max_x;
        if (new_y > max_y) new_y = max_y;
        
        win->x = new_x;
        win->y = new_y;
    }
}

window_manager_t* wm_get_state() {
    return &wm;
}
