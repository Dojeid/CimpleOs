#include "gui/apps/file_explorer.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "fs/vfs.h"
#include "lib/string.h"
#include "lib/printf.h"

static void file_explorer_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 8;
    int y = win->y + 32;

    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x181824);
    draw_string(x, y, 0x38BDF8, "Falkon VFS File Manager: /");
    draw_rect(x, y + 16, win->width - 16, 2, 0x334155);

    vfs_node_t* root = vfs_get_root();
    int line_y = y + 26;

    for (uint32_t i = 0; i < root->child_count; i++) {
        vfs_node_t* child = root->children[i];
        if (!child) continue;

        if (child->type == VFS_DIRECTORY) {
            char dir_hdr[64];
            sprintf(dir_hdr, "[DIR]  /%s/", child->name);
            draw_string(x, line_y, 0xFACC15, dir_hdr);
            line_y += 18;

            for (uint32_t j = 0; j < child->child_count; j++) {
                vfs_node_t* sub = child->children[j];
                if (!sub) continue;
                char line[64];
                if (sub->type == VFS_DIRECTORY) {
                    sprintf(line, "    |-- [DIR]  %s/", sub->name);
                    draw_string(x, line_y, 0xEAB308, line);
                } else {
                    sprintf(line, "    |-- [FILE] %-14s (%u B)", sub->name, sub->size);
                    draw_string(x, line_y, 0xF1F5F9, line);
                }
                line_y += 18;
            }
        } else {
            char line[64];
            sprintf(line, "[FILE] %-16s (%u B)", child->name, child->size);
            draw_string(x, line_y, 0xF1F5F9, line);
            line_y += 18;
        }
    }
}

void file_explorer_open(void) {
    window_t* win = wm_create_window(100, 80, 440, 320, "File Explorer");
    if (win) {
        win->render_content = file_explorer_redraw;
        taskbar_add_button(win->id, "Explorer");
        file_explorer_redraw(win);
    }
}
