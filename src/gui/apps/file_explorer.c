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

    dentry_t* root = vfs_get_root();
    
    int line_y = win->y + 45;
    for (uint32_t i = 0; i < root->d_child_count; i++) {
        dentry_t* child = root->d_subdirs[i];
        
        char dir_hdr[64];
        if (child->d_inode && (child->d_inode->i_mode & 0x4000)) {
            // It's a directory
            sprintf(dir_hdr, "[DIR]  /%s/", child->d_name);
            draw_string(win->x + 10, line_y, 0x00FFFF, dir_hdr);
            line_y += 14;
            
            for (uint32_t j = 0; j < child->d_child_count; j++) {
                dentry_t* sub = child->d_subdirs[j];
                char line[64];
                
                if (sub->d_inode && (sub->d_inode->i_mode & 0x4000)) {
                    sprintf(line, "    |-- [DIR]  %s/", sub->d_name);
                    draw_string(win->x + 10, line_y, 0x00CCCC, line);
                } else {
                    sprintf(line, "    |-- [FILE] %-14s (%u B)", sub->d_name, sub->d_inode ? sub->d_inode->i_size : 0);
                    draw_string(win->x + 10, line_y, 0xAAAAAA, line);
                }
                line_y += 14;
            }
        } else {
            // It's a file in root
            char line[64];
            sprintf(line, "[FILE] %-16s (%u B)", child->d_name, child->d_inode ? child->d_inode->i_size : 0);
            draw_string(win->x + 10, line_y, 0xDDDDDD, line);
            line_y += 14;
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
