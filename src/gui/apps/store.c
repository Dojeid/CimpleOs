#include "gui/apps/store.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "fs/vfs.h"
#include "lib/printf.h"
#include "lib/string.h"

typedef struct {
    const char* name;
    const char* version;
    const char* category;
    const char* description;
    uint32_t size_kb;
    int installed;
    int progress; // 0..100
} pkg_item_t;

static pkg_item_t catalog[8] = {
    {"GCC Toolchain",       "13.2.0", "DevTools",   "Native GNU C/C++ Compiler Suite & Linker",         48500, 1, 100},
    {"Python Runtime",      "3.12.2", "DevTools",   "Python 3 Interpreter & Standard Libraries",         24200, 1, 100},
    {"FFmpeg Codec Pack",   "6.1.1",  "Multimedia", "H.264/AAC Audio & Video Decoder Libraries",       32100, 1, 100},
    {"Node.js Runtime",     "20.11",  "DevTools",   "V8 JavaScript Engine & Package Manager (NPM)",      38900, 0, 0},
    {"Git Version Control", "2.44.0", "DevTools",   "Distributed Source Code Management System",         15400, 1, 100},
    {"Doom Engine Port",    "1.10",   "Games",      "Classic 3D Raycasting Renderer Engine Demo",        12800, 0, 0},
    {"Neovim Editor",       "0.9.5",  "Utilities",  "Hyperextensible Vim-based Terminal Code Editor",    8900,  1, 100},
    {"Linux ELF ABI Layer", "6.8.0",  "System",     "Linux ELF Executable Compatibility Subsystem",      5200,  1, 100}
};

static int active_category = 0; // 0=All, 1=DevTools, 2=Multimedia, 3=Installed

static void store_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 8;
    int y = win->y + 32;
    int w = win->width - 16;
    int h = win->height - 40;

    // Dark sleek background (Midnight Slate 0x0F172A)
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0F172A);

    // Title Bar Sub-header
    draw_string(x, y, 0x38BDF8, "Falkon Package Manager & Application Store v2.0");
    draw_rect(x, y + 16, w, 1, 0x334155);

    // Category Tabs
    const char* tabs[4] = {"All Packages", "Dev Tools", "Multimedia", "Installed"};
    int tab_w = 120;
    for (int i = 0; i < 4; i++) {
        int tx = x + i * (tab_w + 8);
        int ty = y + 24;
        uint32_t bg = (active_category == i) ? 0x0284C7 : 0x1E293B;
        uint32_t fg = (active_category == i) ? 0xFFFFFF : 0x94A3B8;
        draw_rect(tx, ty, tab_w, 24, bg);
        draw_string(tx + 12, ty + 5, fg, tabs[i]);
    }

    draw_rect(x, y + 54, w, 1, 0x334155);

    // Package Items List
    int list_y = y + 62;
    int shown = 0;

    for (int i = 0; i < 8; i++) {
        pkg_item_t* pkg = &catalog[i];

        // Filter by tab category
        if (active_category == 1 && strcmp(pkg->category, "DevTools") != 0) continue;
        if (active_category == 2 && strcmp(pkg->category, "Multimedia") != 0) continue;
        if (active_category == 3 && !pkg->installed) continue;

        int item_y = list_y + shown * 48;
        if (item_y + 42 > y + h) break;

        // Card Container
        draw_rect(x, item_y, w, 44, 0x1E293B);
        draw_rect(x, item_y, 4, 44, pkg->installed ? 0x10B981 : 0x38BDF8);

        // Package Name & Version
        char title_buf[64];
        sprintf(title_buf, "%s  v%s", pkg->name, pkg->version);
        draw_string(x + 12, item_y + 6, 0xF1F5F9, title_buf);

        // Category Tag
        draw_rect(x + 280, item_y + 6, 75, 16, 0x334155);
        draw_string(x + 284, item_y + 8, 0x38BDF8, pkg->category);

        // Description
        draw_string(x + 12, item_y + 24, 0x94A3B8, pkg->description);

        // Size in MB
        char size_buf[32];
        sprintf(size_buf, "%u MB", pkg->size_kb / 1024);
        draw_string(x + w - 170, item_y + 14, 0x94A3B8, size_buf);

        // Action Button [ Install ] / [ Installed ]
        int btn_x = x + w - 90;
        int btn_y = item_y + 10;
        if (pkg->installed) {
            draw_rect(btn_x, btn_y, 80, 24, 0x059669);
            draw_string(btn_x + 8, btn_y + 5, 0xFFFFFF, "INSTALLED");
        } else {
            draw_rect(btn_x, btn_y, 80, 24, 0x0284C7);
            draw_string(btn_x + 12, btn_y + 5, 0xFFFFFF, "INSTALL");
        }

        shown++;
    }
}

static void store_handle_click(window_t* win, int rel_x, int rel_y) {
    if (!win) return;

    // Tab Clicks (rel_y ~ 24..48)
    if (rel_y >= 24 && rel_y <= 48) {
        int tab_w = 120;
        for (int i = 0; i < 4; i++) {
            int tx = 8 + i * (tab_w + 8);
            if (rel_x >= tx && rel_x < tx + tab_w) {
                active_category = i;
                store_redraw(win);
                return;
            }
        }
    }

    // List Items Install Clicks (rel_y >= 62)
    if (rel_y >= 62) {
        int w = win->width - 16;
        int btn_x = 8 + w - 90;

        if (rel_x >= btn_x && rel_x <= btn_x + 80) {
            int idx = (rel_y - 62) / 48;
            if (idx >= 0 && idx < 8) {
                catalog[idx].installed = !catalog[idx].installed;
                store_redraw(win);
            }
        }
    }
}

void store_open(void) {
    window_t* win = wm_create_window(80, 50, 640, 400, "Falkon Application Store & Package Manager");
    if (win) {
        win->render_content = store_redraw;
        win->on_click = store_handle_click;
        taskbar_add_button(win->id, "App Store");
        store_redraw(win);
    }
}
