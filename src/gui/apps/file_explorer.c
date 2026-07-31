#include "gui/apps/file_explorer.h"
#include "gui/window_manager.h"
#include "drivers/video/graphics.h"
#include "fs/vfs.h"
#include "lib/string.h"
#include "lib/printf.h"

static void file_explorer_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 8;
    int y = win->y + 32;

    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x1E1E1E);
    draw_string(x, y, 0x00FFCC, "Falkon VFS File Manager: /");
    draw_rect(x, y + 16, win->width - 16, 2, 0x444444);

    vfs_node_t* root = vfs_get_root();
    int line_y = y + 26;

    for (uint32_t i = 0; i < root->child_count; i++) {
        vfs_node_t* child = root->children[i];
        char line[64];
        if (child->type == VFS_DIRECTORY) {
            sprintf(line, "[DIR]  %s/", child->name);
            draw_string(x, line_y, 0xFFFF00, line);
        } else {
            sprintf(line, "[FILE] %-16s (%u bytes)", child->name, child->size);
            draw_string(x, line_y, 0xFFFFFF, line);
        }
        line_y += 18;
    }
}

void file_explorer_open(void) {
    window_t* win = wm_create_window(100, 80, 420, 300, "File Explorer");
    if (win) {
        win->render_content = file_explorer_redraw;
        file_explorer_redraw(win);
    }
}
