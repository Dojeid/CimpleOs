#include "gui/apps/file_explorer.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "gui/apps/notepad.h"
#include "drivers/video/graphics.h"
#include "fs/vfs.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "mm/heap.h"

typedef struct {
    char current_path[128];
    char selected_file[128];
    char status_msg[64];
} explorer_state_t;

static void file_explorer_redraw(window_t* win) {
    if (!win || !win->user_data) return;
    explorer_state_t* state = (explorer_state_t*)win->user_data;

    int x = win->x + 8;
    int y = win->y + 32;

    // Dark sleek background container
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0F172A);

    // Header Address Bar
    char path_hdr[160];
    sprintf(path_hdr, "Falkon VFS File Manager: %s", state->current_path[0] ? state->current_path : "/");
    draw_string(x, y, 0x38BDF8, path_hdr);

    // Toolbar Buttons [ Refresh ] [ New File ] [ Up ]
    int btn_x = win->x + win->width - 165;
    draw_rect(btn_x, y - 2, 75, 20, 0x0284C7);
    draw_string(btn_x + 8, y + 2, 0xFFFFFF, "[ Refresh ]");

    draw_rect(btn_x + 80, y - 2, 75, 20, 0x10B981);
    draw_string(btn_x + 88, y + 2, 0xFFFFFF, "[ + New ]");

    draw_rect(x, y + 18, win->width - 16, 1, 0x334155);

    dentry_t* dir = vfs_lookup(state->current_path[0] ? state->current_path : "/");
    if (!dir) dir = vfs_get_root();

    int line_y = win->y + 55;
    int max_y = win->y + win->height - 30;

    for (uint32_t i = 0; dir && i < dir->d_child_count && line_y < max_y; i++) {
        dentry_t* child = dir->d_subdirs[i];
        if (!child) continue;

        char line[128];
        if (child->d_inode && (child->d_inode->i_mode & 0x4000)) {
            // Directory
            sprintf(line, "[DIR]  /%s/", child->d_name);
            draw_rect(win->x + 10, line_y - 2, win->width - 20, 16, 0x1E293B);
            draw_string(win->x + 16, line_y, 0x38BDF8, line);
        } else {
            // File
            uint32_t sz = child->d_inode ? child->d_inode->i_size : 0;
            sprintf(line, "[FILE] %-20s (%u Bytes)", child->d_name, sz);
            draw_string(win->x + 16, line_y, 0xF1F5F9, line);
        }
        line_y += 18;
    }

    // Footer Status Bar
    draw_rect(win->x + 2, win->y + win->height - 22, win->width - 4, 20, 0x1E293B);
    draw_rect(win->x + 2, win->y + win->height - 22, win->width - 4, 1, 0x334155);
    draw_string(win->x + 12, win->y + win->height - 17, 0x4ADE80, 
                state->status_msg[0] ? state->status_msg : "Click file to open in Notepad | Volume: /dev/sda (EXT4)");
}

static void file_explorer_handle_click(window_t* win, int rel_x, int rel_y) {
    if (!win || !win->user_data) return;
    explorer_state_t* state = (explorer_state_t*)win->user_data;

    // Toolbar Clicks
    int btn_rel_x = win->width - 165;
    if (rel_y >= 5 && rel_y <= 25) {
        if (rel_x >= btn_rel_x && rel_x <= btn_rel_x + 75) {
            // Refresh
            strcpy(state->status_msg, "VFS Directory Refreshed.");
            file_explorer_redraw(win);
            return;
        } else if (rel_x >= btn_rel_x + 80 && rel_x <= btn_rel_x + 155) {
            // New File
            dentry_t* dir = vfs_lookup(state->current_path[0] ? state->current_path : "/");
            const char* default_text = "New File created via Falkon File Explorer.\n";
            vfs_create_file(dir ? dir : vfs_get_root(), "new_doc.txt", (const uint8_t*)default_text, strlen(default_text));
            strcpy(state->status_msg, "Created new_doc.txt in VFS!");
            file_explorer_redraw(win);
            return;
        }
    }

    int content_y = rel_y - 31;
    if (content_y < 0) return;

    int item_idx = content_y / 18;
    dentry_t* dir = vfs_lookup(state->current_path[0] ? state->current_path : "/");
    if (!dir) dir = vfs_get_root();

    if (dir && item_idx >= 0 && (uint32_t)item_idx < dir->d_child_count) {
        dentry_t* child = dir->d_subdirs[item_idx];
        if (child) {
            char full_path[256];
            if (strcmp(state->current_path, "/") == 0) {
                sprintf(full_path, "/%s", child->d_name);
            } else {
                sprintf(full_path, "%s/%s", state->current_path, child->d_name);
            }

            if (child->d_inode && (child->d_inode->i_mode & 0x4000)) {
                // Navigate into Directory
                strncpy(state->current_path, full_path, 127);
                sprintf(state->status_msg, "Entered directory: %s", full_path);
            } else {
                // Open File in Notepad!
                notepad_open(full_path);
                sprintf(state->status_msg, "Opened file in Notepad: %s", child->d_name);
            }
            file_explorer_redraw(win);
        }
    }
}

void file_explorer_open(void) {
    window_t* win = wm_create_window(100, 80, 480, 330, "File Explorer");
    if (!win) return;

    explorer_state_t* state = (explorer_state_t*)kmalloc(sizeof(explorer_state_t));
    if (!state) return;
    memset(state, 0, sizeof(explorer_state_t));
    strcpy(state->current_path, "/");

    win->user_data = state;
    win->render_content = file_explorer_redraw;
    win->on_click = file_explorer_handle_click;
    taskbar_add_button(win->id, "Explorer");
    file_explorer_redraw(win);
}
