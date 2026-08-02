#include "gui/apps/file_explorer.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "gui/apps/notepad.h"
#include "gui/apps/media_player.h"
#include "drivers/video/graphics.h"
#include "fs/vfs.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "mm/heap.h"

typedef struct {
    char current_path[128];
    char selected_name[64];
    char selected_full_path[128];
    int selected_idx;
    int is_dir;
    uint32_t selected_size;
    uint32_t selected_ino;
    char status_msg[128];
} explorer_state_t;

extern int installer_is_system_installed(void);

static void file_explorer_redraw(window_t* win) {
    if (!win || !win->user_data) return;
    explorer_state_t* state = (explorer_state_t*)win->user_data;

    int x = win->x + 8;
    int y = win->y + 30;
    int w = win->width - 16;

    // Dark sleek background container (Midnight Slate 0x0F172A)
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0F172A);

    // 1. Top Navigation Toolbar & Breadcrumbs Bar
    draw_rect(x, y, w, 28, 0x1E293B);
    draw_rect(x, y, w, 1, 0x334155);

    // Back Button [ < Back ]
    draw_rect(x + 4, y + 4, 60, 20, 0x334155);
    draw_string(x + 10, y + 8, 0xFFFFFF, "< Back");

    // Address Bar showing breadcrumb path
    draw_rect(x + 70, y + 4, w - 215, 20, 0x0F172A);
    draw_rect(x + 70, y + 4, w - 215, 1, 0x38BDF8);
    char path_buf[128];
    sprintf(path_buf, "Path: %s", state->current_path[0] ? state->current_path : "/");
    draw_string(x + 76, y + 8, 0x38BDF8, path_buf);

    // Volume Mode Badge: Live ISO vs Installed EXT4 Hard Drive
    int is_installed = installer_is_system_installed();
    draw_rect(x + w - 140, y + 4, 136, 20, is_installed ? 0x059669 : 0x0284C7);
    draw_string(x + w - 134, y + 8, 0xFFFFFF, is_installed ? "/dev/sda1 [EXT4]" : "Live ISO Ramdisk");

    // 2. Multi-Column Header (Name, Type, Size, Inode/Permissions)
    int col_y = y + 32;
    draw_rect(x, col_y, w, 20, 0x1E293B);
    draw_string(x + 10, col_y + 4, 0x94A3B8, "Icon / Name");
    draw_string(x + 180, col_y + 4, 0x94A3B8, "Type");
    draw_string(x + 250, col_y + 4, 0x94A3B8, "Size");
    draw_string(x + 330, col_y + 4, 0x94A3B8, "Inode / Permissions");

    // 3. File & Directory List View Pane
    dentry_t* dir = vfs_lookup(state->current_path[0] ? state->current_path : "/");
    if (!dir) dir = vfs_get_root();

    int line_y = col_y + 24;
    int max_y = win->y + win->height - 75;

    for (uint32_t i = 0; dir && i < dir->d_child_count && line_y < max_y; i++) {
        dentry_t* child = dir->d_subdirs[i];
        if (!child) continue;

        int is_selected = (state->selected_idx == (int)i);
        uint32_t row_bg = is_selected ? 0x0284C7 : ((i % 2 == 0) ? 0x131D31 : 0x0F172A);
        draw_rect(x, line_y - 2, w, 18, row_bg);

        int is_dir = (child->d_inode && (child->d_inode->i_mode & 0x4000));
        uint32_t file_sz = child->d_inode ? child->d_inode->i_size : child->size;
        uint32_t ino_num = child->d_inode ? child->d_inode->i_ino : (i + 1);

        // Icon & File Name
        const char* icon = is_dir ? "[DIR]" : (strstr(child->d_name, ".mp4") ? "[VID]" : (strstr(child->d_name, ".sh") ? "[ELF]" : "[TXT]"));
        uint32_t icon_col = is_dir ? 0x38BDF8 : (strstr(child->d_name, ".mp4") ? 0xF59E0B : 0xF1F5F9);

        char name_buf[40];
        strncpy(name_buf, child->d_name, 24);
        name_buf[24] = '\0';

        char str_name[64];
        sprintf(str_name, "%s %s", icon, name_buf);
        draw_string(x + 10, line_y, icon_col, str_name);

        // Type
        draw_string(x + 180, line_y, 0xCBD5E1, is_dir ? "Folder" : "File");

        // Size
        char sz_buf[32];
        if (is_dir) sprintf(sz_buf, "-");
        else if (file_sz < 1024) sprintf(sz_buf, "%u B", file_sz);
        else sprintf(sz_buf, "%u KB", file_sz / 1024);
        draw_string(x + 250, line_y, 0x4ADE80, sz_buf);

        // Inode & Permissions
        char perm_buf[32];
        sprintf(perm_buf, "ino#%u %s", ino_num, is_dir ? "drwxr-xr-x" : "-rw-r--r--");
        draw_string(x + 330, line_y, 0x94A3B8, perm_buf);

        line_y += 20;
    }

    // 4. File Context Actions Toolbar Bar (Bottom Operations Pane)
    int act_y = win->y + win->height - 50;
    draw_rect(x, act_y, w, 24, 0x1E293B);
    draw_rect(x, act_y, w, 1, 0x334155);

    // Action Buttons: [ Open ] [ Edit Notepad ] [ Play Media ] [ Properties ]
    draw_rect(x + 4, act_y + 2, 60, 20, 0x0284C7);
    draw_string(x + 10, act_y + 6, 0xFFFFFF, "Open");

    draw_rect(x + 70, act_y + 2, 90, 20, 0x10B981);
    draw_string(x + 76, act_y + 6, 0xFFFFFF, "Edit Notepad");

    draw_rect(x + 165, act_y + 2, 85, 20, 0xF59E0B);
    draw_string(x + 171, act_y + 6, 0x000000, "Play Media");

    draw_rect(x + 255, act_y + 2, 80, 20, 0x64748B);
    draw_string(x + 261, act_y + 6, 0xFFFFFF, "Properties");

    // 5. Footer Status Bar
    draw_rect(win->x + 2, win->y + win->height - 22, win->width - 4, 20, 0x0F172A);
    draw_rect(win->x + 2, win->y + win->height - 22, win->width - 4, 1, 0x334155);
    draw_string(win->x + 12, win->y + win->height - 17, 0x4ADE80, 
                state->status_msg[0] ? state->status_msg : "Select a file or directory to view detailed metadata context.");
}

static void file_explorer_handle_click(window_t* win, int rel_x, int rel_y) {
    if (!win || !win->user_data) return;
    explorer_state_t* state = (explorer_state_t*)win->user_data;

    // 1. Back Button Click
    if (rel_y >= 30 && rel_y <= 58) {
        if (rel_x >= 12 && rel_x <= 72) {
            // Navigate to parent directory
            char* last_slash = strrchr(state->current_path, '/');
            if (last_slash && last_slash != state->current_path) {
                *last_slash = '\0';
            } else {
                strcpy(state->current_path, "/");
            }
            state->selected_idx = -1;
            sprintf(state->status_msg, "Navigated up to: %s", state->current_path);
            file_explorer_redraw(win);
            return;
        }
    }

    // 2. Action Toolbar Buttons Clicks
    int act_rel_y = win->height - 50;
    if (rel_y >= act_rel_y && rel_y <= act_rel_y + 24) {
        if (rel_x >= 12 && rel_x <= 72) {
            // Open
            if (state->selected_name[0]) {
                if (state->is_dir) {
                    strncpy(state->current_path, state->selected_full_path, 127);
                    sprintf(state->status_msg, "Entered folder: %s", state->current_path);
                } else if (strstr(state->selected_name, ".mp4") || strstr(state->selected_name, ".avi")) {
                    media_player_open(state->selected_full_path);
                } else {
                    notepad_open(state->selected_full_path);
                }
            }
            file_explorer_redraw(win);
            return;
        } else if (rel_x >= 78 && rel_x <= 168) {
            // Edit Notepad
            if (state->selected_name[0] && !state->is_dir) {
                notepad_open(state->selected_full_path);
            }
            return;
        } else if (rel_x >= 173 && rel_x <= 258) {
            // Play Media
            if (state->selected_name[0]) {
                media_player_open(state->selected_full_path[0] ? state->selected_full_path : "/videos/sample.mp4");
            }
            return;
        } else if (rel_x >= 263 && rel_x <= 343) {
            // Properties
            if (state->selected_name[0]) {
                sprintf(state->status_msg, "Properties: %s | %u Bytes | Inode #%u", state->selected_name, state->selected_size, state->selected_ino);
            }
            file_explorer_redraw(win);
            return;
        }
    }

    // 3. File Row Selection Click
    int list_start_y = 62;
    int content_y = rel_y - list_start_y;
    if (content_y < 0) return;

    int item_idx = content_y / 20;
    dentry_t* dir = vfs_lookup(state->current_path[0] ? state->current_path : "/");
    if (!dir) dir = vfs_get_root();

    if (dir && item_idx >= 0 && (uint32_t)item_idx < dir->d_child_count) {
        dentry_t* child = dir->d_subdirs[item_idx];
        if (child) {
            state->selected_idx = item_idx;
            strncpy(state->selected_name, child->d_name, 63);

            if (strcmp(state->current_path, "/") == 0) {
                sprintf(state->selected_full_path, "/%s", child->d_name);
            } else {
                sprintf(state->selected_full_path, "%s/%s", state->current_path, child->d_name);
            }

            state->is_dir = (child->d_inode && (child->d_inode->i_mode & 0x4000));
            state->selected_size = child->d_inode ? child->d_inode->i_size : child->size;
            state->selected_ino = child->d_inode ? child->d_inode->i_ino : (item_idx + 1);

            sprintf(state->status_msg, "Selected: %s (%s, %u Bytes)", child->d_name, state->is_dir ? "Directory" : "File", state->selected_size);
            file_explorer_redraw(win);
        }
    }
}

void file_explorer_open(void) {
    window_t* win = wm_create_window(90, 60, 520, 360, "Falkon VFS Rich File Explorer");
    if (!win) return;

    explorer_state_t* state = (explorer_state_t*)kmalloc(sizeof(explorer_state_t));
    if (!state) return;
    memset(state, 0, sizeof(explorer_state_t));
    strcpy(state->current_path, "/");
    state->selected_idx = -1;

    win->user_data = state;
    win->render_content = file_explorer_redraw;
    win->on_click = file_explorer_handle_click;
    taskbar_add_button(win->id, "Explorer");
    file_explorer_redraw(win);
}
