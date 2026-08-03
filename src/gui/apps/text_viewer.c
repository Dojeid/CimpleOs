#include "gui/apps/text_viewer.h"
#include "drivers/video/graphics.h"
#include "gui/window_manager.h"
#include "fs/vfs.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "gui/taskbar.h"

static char tv_content[8192];
static int tv_line_count = 0;
static int tv_scroll = 0;
static char* tv_lines[500];

static void tv_render(window_t* win) {
    if (!win) return;
    
    // Background
    draw_rect(win->x, win->y + 32, win->width, win->height - 32, 0x1E1E1E);
    
    int y = win->y + 40;
    int max_visible = (win->height - 50) / 16;
    
    for (int i = tv_scroll; i < tv_line_count && i < tv_scroll + max_visible; i++) {
        draw_string(win->x + 10, y, 0xD4D4D4, tv_lines[i]);
        y += 16;
    }
    
    // Scrollbar track
    int sb_x = win->x + win->width - 15;
    int sb_y = win->y + 32;
    int sb_h = win->height - 32;
    draw_rect(sb_x, sb_y, 15, sb_h, 0x2D2D30);
    
    // Scrollbar thumb
    if (tv_line_count > 0) {
        int thumb_h = (max_visible * sb_h) / tv_line_count;
        if (thumb_h < 20) thumb_h = 20;
        if (thumb_h > sb_h) thumb_h = sb_h;
        int max_scroll = tv_line_count - max_visible;
        if (max_scroll < 0) max_scroll = 0;
        int thumb_y = sb_y;
        if (max_scroll > 0) {
            thumb_y += (tv_scroll * (sb_h - thumb_h)) / max_scroll;
        }
        draw_rect(sb_x + 2, thumb_y, 11, thumb_h, 0x686868);
    }
}

static void tv_keydown(window_t* win, char c, uint8_t scancode) {
    int max_visible = (win->height - 50) / 16;
    if (scancode == 0x48) { // Up
        if (tv_scroll > 0) tv_scroll--;
    } else if (scancode == 0x50) { // Down
        if (tv_scroll < tv_line_count - max_visible) tv_scroll++;
    } else if (scancode == 0x49) { // Page Up
        tv_scroll -= max_visible;
        if (tv_scroll < 0) tv_scroll = 0;
    } else if (scancode == 0x51) { // Page Down
        tv_scroll += max_visible;
        if (tv_scroll > tv_line_count - max_visible) tv_scroll = tv_line_count - max_visible;
        if (tv_scroll < 0) tv_scroll = 0;
    }
    if (win->render_content) win->render_content(win);
}

void text_viewer_open(const char* title, const char* content) {
    tv_scroll = 0;
    tv_line_count = 0;
    
    int len = 0;
    while (content[len] && len < 8191) {
        tv_content[len] = content[len];
        len++;
    }
    tv_content[len] = '\0';
    
    char* ptr = tv_content;
    tv_lines[tv_line_count++] = ptr;
    while (*ptr) {
        if (*ptr == '\n') {
            *ptr = '\0';
            if (tv_line_count < 500) {
                tv_lines[tv_line_count++] = ptr + 1;
            }
        }
        ptr++;
    }
    
    char win_title[64];
    snprintf(win_title, 64, "Text Viewer - %s", title);
    
    window_t* win = wm_create_window(150, 100, 500, 350, win_title);
    if (win) {
        win->render_content = tv_render;
        win->on_keydown = tv_keydown;
        taskbar_add_button(win->id, "Text View");
        tv_render(win);
    }
}

void text_viewer_open_file(const char* vfs_path) {
    vfs_node_t* node = vfs_lookup(vfs_path);
    if (!node) {
        text_viewer_open("Error", "File not found.");
        return;
    }
    if (node->type != 1) { // Assuming 1 is file
        text_viewer_open("Error", "Not a text file.");
        return;
    }
    
    char temp[8192];
    int size = node->size;
    if (size > 8191) size = 8191;
    for (int i = 0; i < size; i++) temp[i] = node->data[i];
    temp[size] = '\0';
    
    text_viewer_open(node->d_name, temp);
}
