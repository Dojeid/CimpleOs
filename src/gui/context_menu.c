#include "gui/context_menu.h"
#include "gui/apps/file_props.h"
#include "drivers/video/graphics.h"
#include "fs/vfs.h"
#include "gui/window_manager.h"
#include "lib/string.h"
#include <stddef.h>

// Forward declarations if not globally available, as requested
extern void settings_open(void);
extern void desktop_init(void);

static context_menu_t g_menu = {0};

void context_menu_clear(void) {
    g_menu.item_count = 0;
    g_menu.hovered_item = -1;
    g_menu.width = 200;
    g_menu.height = 10; // Basic top and bottom padding
}

void context_menu_show(int x, int y) {
    g_menu.x = x;
    g_menu.y = y;
    g_menu.visible = 1;
    g_menu.hovered_item = -1;
    
    // Attempt bounds checking if off screen
    if (g_menu.x + g_menu.width > screen_w) {
        g_menu.x = screen_w - g_menu.width;
    }
    if (g_menu.y + g_menu.height > screen_h) {
        g_menu.y = screen_h - g_menu.height;
    }
}

void context_menu_hide(void) {
    g_menu.visible = 0;
}

int context_menu_is_visible(void) {
    return g_menu.visible;
}

void context_menu_add_item(const char* label, void (*on_select)(void)) {
    if (g_menu.item_count >= CONTEXT_MENU_MAX_ITEMS) return;
    
    context_menu_item_t* item = &g_menu.items[g_menu.item_count++];
    strncpy(item->label, label, sizeof(item->label) - 1);
    item->label[sizeof(item->label) - 1] = '\0';
    item->on_select = on_select;
    item->color = 0xFFFFFFFF; // Default white
    item->separator = 0;
    
    g_menu.height += 24; // 24px per text item
}

void context_menu_add_separator(void) {
    if (g_menu.item_count >= CONTEXT_MENU_MAX_ITEMS) return;
    
    context_menu_item_t* item = &g_menu.items[g_menu.item_count++];
    item->label[0] = '\0';
    item->on_select = NULL;
    item->color = 0;
    item->separator = 1;
    
    g_menu.height += 8; // 8px for separator
}

void context_menu_render(void) {
    if (!g_menu.visible) return;
    
    // Background: Glassmorphic panel
    draw_rounded_rect_alpha(g_menu.x, g_menu.y, g_menu.width, g_menu.height, 8, 0x202020, 200);
    
    int current_y = g_menu.y + 5; 
    
    for (int i = 0; i < g_menu.item_count; i++) {
        context_menu_item_t* item = &g_menu.items[i];
        
        if (item->separator) {
            // Draw 1px line
            draw_rect(g_menu.x + 10, current_y + 3, g_menu.width - 20, 1, 0x606060);
            current_y += 8;
        } else {
            if (i == g_menu.hovered_item) {
                // Highlighted rect
                draw_rounded_rect(g_menu.x + 5, current_y, g_menu.width - 10, 24, 4, 0x404040);
            }
            
            // Text at offset
            draw_string_shadow(g_menu.x + 15, current_y + 6, item->color, 0x000000, item->label);
            current_y += 24;
        }
    }
}

int context_menu_handle_click(int x, int y) {
    if (!g_menu.visible) return 0;
    
    if (x >= g_menu.x && x <= g_menu.x + g_menu.width &&
        y >= g_menu.y && y <= g_menu.y + g_menu.height) {
        
        if (g_menu.hovered_item >= 0 && g_menu.hovered_item < g_menu.item_count) {
            context_menu_item_t* item = &g_menu.items[g_menu.hovered_item];
            if (!item->separator && item->on_select) {
                item->on_select();
            }
        }
        
        context_menu_hide();
        return 1;
    }
    
    context_menu_hide();
    return 0;
}

int context_menu_handle_hover(int x, int y) {
    if (!g_menu.visible) return 0;
    
    if (x >= g_menu.x && x <= g_menu.x + g_menu.width &&
        y >= g_menu.y && y <= g_menu.y + g_menu.height) {
        
        int current_y = g_menu.y + 5;
        g_menu.hovered_item = -1;
        
        for (int i = 0; i < g_menu.item_count; i++) {
            context_menu_item_t* item = &g_menu.items[i];
            int item_height = item->separator ? 8 : 24;
            
            if (y >= current_y && y < current_y + item_height) {
                if (!item->separator) {
                    g_menu.hovered_item = i;
                }
                break;
            }
            
            current_y += item_height;
        }
        return 1;
    }
    
    g_menu.hovered_item = -1;
    return 0;
}

// -----------------------------------------
// Callbacks for Desktop & Windows
// -----------------------------------------

static int target_win_id = -1;

static void desktop_refresh_cb(void) {
    desktop_init();
}

static void open_settings_cb(void) {
    settings_open();
}

static void new_folder_cb(void) {
    vfs_mkdir(vfs_get_root(), "New_Folder");
}

static void about_os_cb(void) {
    file_props_open("/");
}

void context_menu_setup_desktop(void) {
    context_menu_clear();
    context_menu_add_item("Refresh Desktop", desktop_refresh_cb);
    context_menu_add_item("Display Settings", open_settings_cb);
    context_menu_add_item("New Folder", new_folder_cb);
    context_menu_add_separator();
    context_menu_add_item("System Properties", about_os_cb);
}

static void win_minimize_cb(void) {
    if (target_win_id >= 0) wm_minimize_window(target_win_id);
}

static void win_maximize_cb(void) {
    if (target_win_id >= 0) wm_maximize_window(target_win_id);
}

static void win_close_cb(void) {
    if (target_win_id >= 0) wm_destroy_window(target_win_id);
}

static void win_props_cb(void) {
    if (target_win_id >= 0) {
        window_t* win = wm_get_window(target_win_id);
        if (win) {
            file_props_open(win->title[0] ? win->title : "/");
        }
    }
}

void context_menu_setup_window(int win_id) {
    target_win_id = win_id;
    context_menu_clear();
    context_menu_add_item("Minimize", win_minimize_cb);
    context_menu_add_item("Maximize", win_maximize_cb);
    context_menu_add_item("Close Window", win_close_cb);
    context_menu_add_separator();
    context_menu_add_item("Properties", win_props_cb);
}
