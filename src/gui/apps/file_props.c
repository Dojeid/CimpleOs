#include "gui/apps/file_props.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "drivers/rtc.h"
#include "fs/vfs.h"
#include "mm/heap.h"
#include "lib/string.h"
#include "lib/printf.h"

typedef struct {
    char path[128];
    char name[64];
    uint32_t size;
    int is_dir;
    uint32_t ino;
    char date_str[32];
} file_props_info_t;

static void file_props_redraw(window_t* win) {
    if (!win || !win->user_data) return;
    file_props_info_t* info = (file_props_info_t*)win->user_data;

    int x = win->x + 12;
    int y = win->y + 36;

    // Dark sleek container
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0F172A);

    // Title / Header
    draw_string_shadow(x, y, 0x38BDF8, 0x000000, "File & Folder Properties");
    draw_rect(x, y + 16, win->width - 24, 1, 0x334155);

    // Icon box
    int icon_bg = info->is_dir ? 0xF59E0B : 0x0284C7;
    draw_rounded_rect(x + 10, y + 26, 48, 44, 8, icon_bg);
    draw_string_shadow(x + 18, y + 40, 0xFFFFFF, 0x000000, info->is_dir ? "DIR" : "FILE");

    // Name label
    draw_string_shadow(x + 70, y + 32, 0xFFFFFF, 0x000000, info->name);
    draw_string(x + 70, y + 48, 0x94A3B8, info->is_dir ? "Directory Folder" : "Native VFS File");

    draw_rect(x, y + 78, win->width - 24, 1, 0x334155);

    // Details Grid
    int dy = y + 88;
    char line[128];

    // Location
    draw_string(x + 10, dy, 0x94A3B8, "Location:");
    draw_string(x + 110, dy, 0xF1F5F9, info->path);

    // Size
    dy += 20;
    draw_string(x + 10, dy, 0x94A3B8, "Size:");
    if (info->is_dir) {
        sprintf(line, "-- (Directory)");
    } else {
        sprintf(line, "%u bytes (%u KB)", info->size, (info->size + 1023) / 1024);
    }
    draw_string(x + 110, dy, 0x4ADE80, line);

    // File System
    dy += 20;
    draw_string(x + 10, dy, 0x94A3B8, "Filesystem:");
    draw_string(x + 110, dy, 0x38BDF8, "Virtual VFS / EXT4 Linux Native");

    // Inode / Node ID
    dy += 20;
    draw_string(x + 10, dy, 0x94A3B8, "Node ID:");
    sprintf(line, "0x%08X", info->ino);
    draw_string(x + 110, dy, 0xE2E8F0, line);

    // Permissions
    dy += 20;
    draw_string(x + 10, dy, 0x94A3B8, "Permissions:");
    draw_string(x + 110, dy, 0x22C55E, info->is_dir ? "drwxr-xr-x (0755)" : "-rw-r--r-- (0644)");

    // Date
    dy += 20;
    draw_string(x + 10, dy, 0x94A3B8, "Created:");
    draw_string(x + 110, dy, 0x94A3B8, info->date_str[0] ? info->date_str : "2026-08-03 (RTC Sync)");

    // OK Button
    int btn_w = 80, btn_h = 24;
    int btn_x = win->x + (win->width - btn_w) / 2;
    int btn_y = win->y + win->height - 34;
    draw_rounded_rect(btn_x, btn_y, btn_w, btn_h, 6, 0x0284C7);
    draw_string(btn_x + 30, btn_y + 5, 0xFFFFFF, "OK");
}

static void file_props_click(window_t* win, int rel_x, int rel_y) {
    if (!win) return;
    int btn_w = 80, btn_h = 24;
    int btn_x = (win->width - btn_w) / 2;
    int btn_y = win->height - 34;

    if (rel_x >= btn_x && rel_x <= btn_x + btn_w && rel_y >= btn_y && rel_y <= btn_y + btn_h) {
        wm_destroy_window(win->id);
    }
}

void file_props_open(const char* vfs_path) {
    if (!vfs_path) vfs_path = "/";

    vfs_node_t* node = vfs_lookup(vfs_path);
    file_props_info_t* info = (file_props_info_t*)malloc(sizeof(file_props_info_t));
    if (!info) return;

    memset(info, 0, sizeof(file_props_info_t));
    strcpy(info->path, vfs_path);

    if (node) {
        strcpy(info->name, node->d_name[0] ? node->d_name : "/");
        info->size = node->size;
        info->is_dir = (node->type == VFS_DIRECTORY);
        info->ino = (uint32_t)(uintptr_t)node;
    } else {
        strcpy(info->name, "Unknown");
        info->size = 0;
        info->is_dir = 0;
    }

    rtc_time_t t;
    rtc_read(&t);
    sprintf(info->date_str, "%04u-%02u-%02u %02u:%02u:%02u",
            t.year, t.month, t.day, t.hours, t.minutes, t.seconds);

    window_t* win = wm_create_window(220, 120, 360, 260, "Properties");
    if (win) {
        win->user_data = info;
        win->render_content = file_props_redraw;
        win->on_click = file_props_click;
        taskbar_add_button(win->id, "Properties");
        file_props_redraw(win);
    }
}
