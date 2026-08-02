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
    win->alpha_anim = 0;
    win->anim_scale = 90;
    win->accent_glow_color = 0x38BDF8;
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
    int cx = win->x + win->width - 18;
    int cy = win->y + TITLEBAR_HEIGHT / 2;
    int dx = x - cx, dy = y - cy;
    return (dx*dx + dy*dy <= 8*8);
}

int wm_is_point_in_minimize_button(window_t* win, int x, int y) {
    if (!win) return 0;
    int cx = win->x + win->width - 54;
    int cy = win->y + TITLEBAR_HEIGHT / 2;
    int dx = x - cx, dy = y - cy;
    return (dx*dx + dy*dy <= 8*8);
}

int wm_is_point_in_maximize_button(window_t* win, int x, int y) {
    if (!win) return 0;
    int cx = win->x + win->width - 36;
    int cy = win->y + TITLEBAR_HEIGHT / 2;
    int dx = x - cx, dy = y - cy;
    return (dx*dx + dy*dy <= 8*8);
}

void wm_render_window(window_t* win) {
    if (!win || !(win->flags & WIN_FLAG_VISIBLE) ||
        (win->flags & WIN_FLAG_MINIMIZED)) return;

    // ── Open animation ──────────────────────────────────────
    if (win->alpha_anim < 255) {
        if (win->alpha_anim + 40 >= 255) win->alpha_anim = 255;
        else win->alpha_anim += 40;
    }
    if (win->anim_scale < 100) win->anim_scale += 6;

    int is_focused = (win->flags & WIN_FLAG_FOCUSED);
    gui_theme_t* theme = theme_get_current();

    int wx = win->x, wy = win->y;
    int ww = win->width, wh_total = TITLEBAR_HEIGHT + win->height;
    int corner_r = 8;

    // ── 1. Multi-pass Soft Shadow ───────────────────────────
    draw_window_shadow(wx, wy, ww, wh_total);

    // ── 2. Mica Box-Blur (sample desktop wallpaper pixels) ──
    draw_box_blur(wx, wy, ww, TITLEBAR_HEIGHT, 4);

    // ── 3. Rounded Window Body — dark content bg ────────────
    // Draw content area first (below titlebar)
    draw_rounded_rect(wx, wy, ww, wh_total, corner_r, 0x1A1F2E);

    // ── 4. Acrylic Titlebar Overlay ─────────────────────────
    // Top half of window: glassmorphic acrylic tint over blur
    uint32_t tb_color = is_focused ? 0x1E2A3A : 0x151B26;
    uint8_t  tb_alpha = is_focused ? 200 : 160;
    draw_rounded_rect_alpha(wx, wy, ww, TITLEBAR_HEIGHT, corner_r, tb_color, tb_alpha);

    // Sharp bottom edge of titlebar (blends into content)
    draw_rect(wx, wy + TITLEBAR_HEIGHT - 1, ww, 1, 0x0D1117);

    // ── 5. Title: Small App Icon Box + Centered Title Text ──
    // Left icon badge (4×4 rounded)
    uint32_t icon_col = is_focused ? theme->accent_color : 0x4B5563;
    draw_rounded_rect(wx + 10, wy + TITLEBAR_HEIGHT/2 - 5, 10, 10, 3, icon_col);

    // Title text centered in titlebar
    int title_len = 0;
    const char* tp = win->title;
    while (*tp++) title_len++;
    int text_px_w = title_len * 8;
    int text_x = wx + ww/2 - text_px_w/2;
    int text_y = wy + TITLEBAR_HEIGHT/2 - 4;
    draw_string_shadow(text_x, text_y,
        is_focused ? 0xF1F5F9 : 0x6B7280,
        0x000000,
        win->title);

    // ── 6. Traffic-Light Control Buttons (circles) ──────────
    int btn_cy = wy + TITLEBAR_HEIGHT/2;    // vertical center
    int btn_r  = 6;

    // Close  — red circle (rightmost)
    int close_cx = wx + ww - 18;
    draw_circle(close_cx, btn_cy, btn_r, is_focused ? 0xEF4444 : 0x5C2626);
    if (is_focused) {
        // "×" inside
        draw_string(close_cx - 3, btn_cy - 4, 0xFFFFFF, "x");
    }

    // Maximize — green circle
    int max_cx = wx + ww - 36;
    draw_circle(max_cx, btn_cy, btn_r, is_focused ? 0x22C55E : 0x1A4229);
    if (is_focused) draw_string(max_cx - 3, btn_cy - 4, 0xFFFFFF, "+");

    // Minimize — yellow circle
    int min_cx = wx + ww - 54;
    draw_circle(min_cx, btn_cy, btn_r, is_focused ? 0xF59E0B : 0x4D3508);
    if (is_focused) draw_string(min_cx - 3, btn_cy - 4, 0x7C4A00, "-");

    // ── 7. Content Area ─────────────────────────────────────
    // Already filled by rounded rect above.
    // Thin separator between titlebar and content
    draw_rect(wx + 2, wy + TITLEBAR_HEIGHT, ww - 4, 1, 0x0D1117);

    if (win->render_content) win->render_content(win);

    // ── 8. Window Accent Border (rounded outline) ───────────
    uint32_t border_col = is_focused
        ? (theme->accent_color ? theme->accent_color : 0x38BDF8)
        : 0x1F2937;
    draw_rounded_rect_outline(wx, wy, ww, wh_total, corner_r, 1, border_col);

    // Extra inner glow on focused
    if (is_focused)
        draw_rounded_rect_outline(wx+1, wy+1, ww-2, wh_total-2, corner_r-1, 1, border_col & 0x7FFFFFFF);
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
    extern int screen_w, screen_h;
    
    // Stop dragging and check window snapping
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (wm.windows[i].id != -1 && (wm.windows[i].flags & WIN_FLAG_DRAGGING)) {
            window_t* win = &wm.windows[i];
            win->flags &= ~WIN_FLAG_DRAGGING;

            // Window Edge Snapping
            if (x <= 15) {
                // Snap Left Half
                win->x = 0;
                win->y = 24;
                win->width = screen_w / 2;
                win->height = screen_h - 54;
            } else if (x >= screen_w - 15) {
                // Snap Right Half
                win->x = screen_w / 2;
                win->y = 24;
                win->width = screen_w / 2;
                win->height = screen_h - 54;
            } else if (y <= 26) {
                // Snap Top / Maximize
                win->x = 0;
                win->y = 24;
                win->width = screen_w;
                win->height = screen_h - 54;
            }
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
        
        extern int screen_w, screen_h;

        // Windows 11 Snap Ghost Preview Outline while dragging near edges
        if (x <= 20) {
            // Snap Left ghost
            draw_rect_alpha(0, 24, screen_w / 2, screen_h - 54, 0x38BDF8, 40);
            draw_rect(0, 24, screen_w / 2, 2, 0x38BDF8);
            draw_rect(0, 24, 2, screen_h - 54, 0x38BDF8);
            draw_rect(screen_w / 2 - 2, 24, 2, screen_h - 54, 0x38BDF8);
        } else if (x >= screen_w - 20) {
            // Snap Right ghost
            draw_rect_alpha(screen_w / 2, 24, screen_w / 2, screen_h - 54, 0x38BDF8, 40);
            draw_rect(screen_w / 2, 24, screen_w / 2, 2, 0x38BDF8);
            draw_rect(screen_w / 2, 24, 2, screen_h - 54, 0x38BDF8);
            draw_rect(screen_w - 2, 24, 2, screen_h - 54, 0x38BDF8);
        } else if (y <= 28) {
            // Snap Maximize ghost
            draw_rect_alpha(0, 24, screen_w, screen_h - 54, 0x38BDF8, 30);
            draw_rect(0, 24, screen_w, 2, 0x38BDF8);
        }
        
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
