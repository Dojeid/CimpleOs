#include "gui/apps/notepad.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "fs/vfs.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "mm/heap.h"
#include "kernel/timer.h"

typedef struct {
    char filepath[128];
    char filename[64];
    char content[2048];
    int len;
    char status_msg[64];
} notepad_state_t;

static void notepad_render(window_t* win) {
    if (!win || !win->user_data) return;
    notepad_state_t* state = (notepad_state_t*)win->user_data;

    // Background container (Dark Obsidian 0x0F172A)
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x0F172A);
    
    // Header & Toolbar
    char title[128];
    sprintf(title, "Falkon Text Editor - %s", state->filename[0] ? state->filename : "untitled.txt");
    draw_string(win->x + 12, win->y + 30, 0x38BDF8, title);
    
    // Interactive Save Button [ Save File ]
    int save_btn_x = win->x + win->width - 115;
    int save_btn_y = win->y + 27;
    draw_rect(save_btn_x, save_btn_y, 100, 20, 0x0284C7);
    draw_string(save_btn_x + 10, save_btn_y + 4, 0xFFFFFF, "[ Save Disk ]");

    draw_rect(win->x + 10, win->y + 50, win->width - 20, 1, 0x334155);

    // Left Gutter / Line Numbers Bar
    draw_rect(win->x + 10, win->y + 51, 35, win->height - 76, 0x1E293B);
    draw_rect(win->x + 45, win->y + 51, 1, win->height - 76, 0x334155);

    int cur_x = win->x + 52;
    int cur_y = win->y + 56;
    int line_num = 1;
    int max_x = win->x + win->width - 16;
    int max_y = win->y + win->height - 30;

    // Draw Line Number 1
    char line_str[16];
    sprintf(line_str, "%2d", line_num);
    draw_string(win->x + 14, cur_y, 0x94A3B8, line_str);

    // Render Editable Content
    for (int i = 0; i < state->len && cur_y < max_y; i++) {
        char c = state->content[i];

        if (c == '\n' || cur_x >= max_x) {
            cur_x = win->x + 52;
            cur_y += 15;
            line_num++;
            if (cur_y < max_y) {
                sprintf(line_str, "%2d", line_num);
                draw_string(win->x + 14, cur_y, 0x94A3B8, line_str);
            }
            if (c == '\n') continue;
        }

        draw_char(cur_x, cur_y, c, 0xF1F5F9);
        cur_x += 8;
    }

    // Blinking Cursor
    extern volatile uint32_t timer_ticks;
    if ((timer_ticks / 30) % 2 == 0 && cur_y < max_y) {
        draw_rect(cur_x, cur_y, 8, 12, 0x38BDF8);
    }

    // Footer Status Bar
    draw_rect(win->x + 2, win->y + win->height - 22, win->width - 4, 20, 0x1E293B);
    draw_rect(win->x + 2, win->y + win->height - 22, win->width - 4, 1, 0x334155);

    char stat_str[128];
    sprintf(stat_str, "%s | %d chars | Line %d | UTF-8 EXT4", 
            state->status_msg[0] ? state->status_msg : "Ready", state->len, line_num);
    draw_string(win->x + 12, win->y + win->height - 17, 0x4ADE80, stat_str);
}

static void notepad_handle_keydown(window_t* win, char c, uint8_t scancode) {
    if (!win || !win->user_data) return;
    notepad_state_t* state = (notepad_state_t*)win->user_data;

    if (c == '\b') {
        if (state->len > 0) {
            state->len--;
            state->content[state->len] = '\0';
            strcpy(state->status_msg, "Modified");
        }
    }
    else if (c == '\n') {
        if (state->len < 2045) {
            state->content[state->len++] = '\n';
            state->content[state->len] = '\0';
            strcpy(state->status_msg, "Modified");
        }
    }
    else if (c >= 32 && c <= 126) {
        if (state->len < 2045) {
            state->content[state->len++] = c;
            state->content[state->len] = '\0';
            strcpy(state->status_msg, "Modified");
        }
    }

    notepad_render(win);
}

static void notepad_handle_click(window_t* win, int rel_x, int rel_y) {
    if (!win || !win->user_data) return;
    notepad_state_t* state = (notepad_state_t*)win->user_data;

    // Check click on [ Save Disk ] button
    int save_btn_rel_x = win->width - 115;
    if (rel_x >= save_btn_rel_x && rel_x <= save_btn_rel_x + 100 && rel_y >= 5 && rel_y <= 25) {
        dentry_t* file = vfs_lookup(state->filepath[0] ? state->filepath : "/docs/welcome.txt");
        if (file && file->d_inode) {
            if (file->d_inode->i_private) kfree(file->d_inode->i_private);
            file->d_inode->i_size = state->len;
            file->d_inode->i_private = kmalloc(state->len + 1);
            memcpy(file->d_inode->i_private, state->content, state->len);
            ((char*)file->d_inode->i_private)[state->len] = '\0';
            strcpy(state->status_msg, "SAVED TO EXT4 DISK!");
        } else {
            dentry_t* root = vfs_lookup("/");
            vfs_create_file(root, "welcome.txt", (const uint8_t*)state->content, state->len);
            strcpy(state->status_msg, "FILE CREATED & SAVED!");
        }
        notepad_render(win);
    }
}

void notepad_open(const char* filepath) {
    window_t* win = wm_create_window(180, 100, 520, 340, "Falkon Text Editor");
    if (!win) return;

    notepad_state_t* state = (notepad_state_t*)kmalloc(sizeof(notepad_state_t));
    if (!state) return;
    memset(state, 0, sizeof(notepad_state_t));

    const char* path = filepath ? filepath : "/docs/welcome.txt";
    strncpy(state->filepath, path, 127);

    dentry_t* file = vfs_lookup(path);
    if (file && file->d_name[0]) {
        strncpy(state->filename, file->d_name, 63);
    } else {
        strcpy(state->filename, "welcome.txt");
    }

    if (file && file->d_inode && file->d_inode->i_private) {
        const char* text = (const char*)file->d_inode->i_private;
        strncpy(state->content, text, 2047);
        state->len = strlen(state->content);
    } else {
        const char* default_text = "Welcome to Falkon-OS Text Editor v1.0!\nType content directly using your keyboard.\nClick [ Save Disk ] to write changes to EXT4 storage.";
        strncpy(state->content, default_text, 2047);
        state->len = strlen(state->content);
    }

    strcpy(state->status_msg, "File Loaded");

    win->user_data = state;
    win->render_content = notepad_render;
    win->on_keydown = notepad_handle_keydown;
    win->on_click = notepad_handle_click;
    taskbar_add_button(win->id, "Notepad");
    notepad_render(win);
}
