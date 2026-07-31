#include "gui/apps/notepad.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "fs/vfs.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "mm/heap.h"

typedef struct {
    const char* filename;
    const char* content;
} notepad_state_t;

static void notepad_redraw(window_t* win, const char* filename, const char* content) {
    if (!win) return;
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x111122);
    
    char title[64];
    sprintf(title, "Falkon Notepad - %s", filename ? filename : "welcome.txt");
    draw_string(win->x + 10, win->y + 30, 0x00FF00, title);
    draw_rect(win->x + 10, win->y + 44, win->width - 20, 2, 0x333366);

    int cur_x = win->x + 12;
    int cur_y = win->y + 54;
    int max_x = win->x + win->width - 16;

    for (int i = 0; content && content[i] != 0; i++) {
        if (content[i] == '\n' || cur_x >= max_x) {
            cur_x = win->x + 12;
            cur_y += 14;
            if (content[i] == '\n') continue;
        }
        draw_char(cur_x, cur_y, content[i], 0xFFFFFF);
        cur_x += 8;
    }
}

static void notepad_render(window_t* win) {
    if (!win || !win->user_data) return;
    notepad_state_t* state = (notepad_state_t*)win->user_data;
    notepad_redraw(win, state->filename, state->content);
}

void notepad_open(const char* filepath) {
    window_t* win = wm_create_window(200, 120, 480, 320, "Notepad");
    if (!win) return;

    vfs_node_t* file = vfs_lookup(0, filepath ? filepath : "/docs/welcome.txt");
    const char* text = (file && file->data) ? (const char*)file->data : "Welcome to Falkon-OS Text Editor!\nType commands in Terminal or explore VFS files.";

    notepad_state_t* state = (notepad_state_t*)malloc(sizeof(notepad_state_t));
    if (!state) return;
    state->filename = file ? file->name : "welcome.txt";
    state->content = text;

    win->user_data = state;
    win->render_content = notepad_render;
    taskbar_add_button(win->id, "Notepad");
    notepad_render(win);
}
