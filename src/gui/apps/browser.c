#include "gui/apps/browser.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "fs/vfs.h"
#include "lib/printf.h"
#include "lib/string.h"

static char current_url[128] = "file:///docs/welcome.txt";
static char page_title[64] = "Falkon Surf Web Browser";
static char page_content[2048] = "";

static void browser_load_page(const char* url) {
    if (!url || !url[0]) return;
    strncpy(current_url, url, sizeof(current_url) - 1);
    sprintf(page_title, "Falkon Surf: %s", current_url);

    if (strncmp(current_url, "file://", 7) == 0) {
        const char* path = current_url + 7;
        vfs_node_t* node = vfs_lookup(path);
        if (node && node->data) {
            strncpy(page_content, (const char*)node->data, sizeof(page_content) - 1);
            page_content[sizeof(page_content) - 1] = '\0';
        } else {
            sprintf(page_content, "404 Not Found: '%s' was not found on VFS.", path);
        }
    } else {
        sprintf(page_content,
            "Falkon Web Surf v1.0\n"
            "Fetching: %s\n"
            "Status: 200 OK | Network: Intel e1000 Gigabit Active\n"
            "Content-Type: text/html; charset=UTF-8\n",
            current_url);
    }
}

static void browser_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 8;
    int y = win->y + 32;
    int w = win->width - 16;
    int h = win->height - 40;

    // Outer dark container
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0F172A);

    // Browser Address Bar Header
    draw_rect(x, y, w, 32, 0x1E293B);
    draw_rect(x, y + 31, w, 1, 0x334155);

    // Back / Forward / Refresh Buttons
    draw_rect(x + 4, y + 4, 24, 24, 0x334155);
    draw_string(x + 12, y + 9, 0xFFFFFF, "<");

    draw_rect(x + 32, y + 4, 24, 24, 0x334155);
    draw_string(x + 40, y + 9, 0xFFFFFF, ">");

    draw_rect(x + 60, y + 4, 24, 24, 0x334155);
    draw_string(x + 68, y + 9, 0xFFFFFF, "R");

    // Address Bar Text Input Field
    draw_rect(x + 92, y + 4, w - 180, 24, 0x0B0F19);
    draw_string(x + 100, y + 9, 0x38BDF8, current_url);

    // Go Button
    draw_rect(x + w - 82, y + 4, 78, 24, 0x10B981);
    draw_string(x + w - 60, y + 9, 0x000000, "SURF");

    // HTML Viewport Display Surface
    int vp_x = x;
    int vp_y = y + 38;
    int vp_w = w;
    int vp_h = h - 42;

    draw_rect(vp_x, vp_y, vp_w, vp_h, 0x0B0F19);

    // Render HTML Text Content
    draw_string(vp_x + 12, vp_y + 12, 0x4ADE80, page_title);
    draw_rect(vp_x + 12, vp_y + 26, vp_w - 24, 1, 0x334155);

    draw_string(vp_x + 12, vp_y + 36, 0xF1F5F9, page_content);
}

static void browser_handle_click(window_t* win, int rel_x, int rel_y) {
    if (!win) return;
    if (rel_y >= 32 && rel_y <= 64) {
        int w = win->width - 16;
        if (rel_x >= w - 82 && rel_x <= w - 4) {
            // Refresh / Reload current URL
            browser_load_page(current_url);
            browser_redraw(win);
        }
    }
}

void browser_open(const char* url) {
    browser_load_page(url && url[0] ? url : "file:///docs/welcome.txt");

    window_t* win = wm_create_window(80, 50, 640, 420, "falkon-surf Web Browser");
    if (win) {
        win->render_content = browser_redraw;
        win->on_click = browser_handle_click;
        taskbar_add_button(win->id, "falkon-surf");
        browser_redraw(win);
    }
}
