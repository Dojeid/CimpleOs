#include "gui/apps/code_editor.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "fs/vfs.h"
#include "lib/printf.h"
#include "lib/string.h"

static char editor_file[128] = "/src/main.c";
static char editor_content[4096] = 
    "// Falkon-OS Native C Application\n"
    "#include <stdio.h>\n"
    "\n"
    "int main(int argc, char** argv) {\n"
    "    printf(\"Hello from Falkon-OS User-Mode Ring 3!\\n\");\n"
    "    return 0;\n"
    "}\n";

static void code_editor_redraw(window_t* win) {
    if (!win) return;
    int x = win->x + 8;
    int y = win->y + 32;
    int w = win->width - 16;
    int h = win->height - 40;

    // Dark glass container
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0A0E17);

    // IDE Header Tabs Bar
    draw_rect(x, y, w, 28, 0x1E293B);
    draw_rect(x, y + 27, w, 1, 0x334155);

    // Active File Tab
    draw_rect(x + 4, y + 2, 140, 24, 0x38BDF8);
    draw_string(x + 12, y + 7, 0x000000, "main.c [C Source]");

    draw_rect(x + 148, y + 2, 130, 24, 0x0F172A);
    draw_string(x + 156, y + 7, 0x94A3B8, "syscall.h");

    // Action Toolbar [ BUILD ], [ RUN ], [ SAVE ]
    draw_rect(x + w - 190, y + 3, 58, 22, 0x10B981);
    draw_string(x + w - 182, y + 7, 0x000000, "BUILD");

    draw_rect(x + w - 128, y + 3, 58, 22, 0x38BDF8);
    draw_string(x + w - 116, y + 7, 0x000000, "RUN");

    draw_rect(x + w - 66, y + 3, 60, 22, 0xF59E0B);
    draw_string(x + w - 56, y + 7, 0x000000, "SAVE");

    // Code Line Number Gutter & Editor Viewport
    int gutter_w = 40;
    int editor_x = x + gutter_w;
    int editor_w = w - gutter_w;
    int editor_y = y + 32;
    int editor_h = h - 36;

    // Line Numbers Gutter
    draw_rect(x, editor_y, gutter_w, editor_h, 0x111827);
    draw_rect(x + gutter_w - 1, editor_y, 1, editor_h, 0x334155);

    for (int line = 1; line <= 18; line++) {
        char lnum[8];
        sprintf(lnum, "%2d", line);
        draw_string(x + 10, editor_y + ((line - 1) * 16) + 4, 0x64748B, lnum);
    }

    // Code Editor Text Area
    draw_rect(editor_x, editor_y, editor_w, editor_h, 0x0B0F19);
    draw_string(editor_x + 10, editor_y + 4, 0x38BDF8, editor_content);
}

void code_editor_open(const char* file_path) {
    if (file_path && file_path[0]) strncpy(editor_file, file_path, sizeof(editor_file) - 1);

    window_t* win = wm_create_window(95, 65, 660, 430, "falkon-code IDE");
    if (win) {
        win->render_content = code_editor_redraw;
        taskbar_add_button(win->id, "falkon-code");
        code_editor_redraw(win);
    }
}
