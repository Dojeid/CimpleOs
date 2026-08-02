#include "gui/apps/calc.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "mm/heap.h"

typedef struct {
    char display[32];
    long val1;
    long val2;
    char op;
    int clear_on_next;
} calc_state_t;

static void calc_redraw(window_t* win) {
    if (!win || !win->user_data) return;
    calc_state_t* state = (calc_state_t*)win->user_data;

    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x1E1E2E);

    // Calculator LCD Display Window
    draw_rect(win->x + 10, win->y + 32, win->width - 20, 32, 0x0F0F17);
    draw_rect(win->x + 10, win->y + 32, win->width - 20, 1, 0x38BDF8);

    // Right-align LCD text
    int text_len = strlen(state->display);
    int start_x = win->x + win->width - 25 - (text_len * 8);
    if (start_x < win->x + 16) start_x = win->x + 16;
    draw_string(start_x, win->y + 42, 0x38BDF8, state->display);

    // Keypad Grid Layout
    const char* keys[4][4] = {
        {"7", "8", "9", "/"},
        {"4", "5", "6", "*"},
        {"1", "2", "3", "-"},
        {"C", "0", "=", "+"}
    };

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int bx = win->x + 12 + c * 52;
            int by = win->y + 72 + r * 38;
            uint32_t bg = (c == 3) ? 0xF59E0B : ((r == 3 && c == 0) ? 0xEF4444 : ((r == 3 && c == 2) ? 0x10B981 : 0x334155));
            draw_rect(bx, by, 46, 32, bg);
            draw_string(bx + 18, by + 10, 0xFFFFFF, keys[r][c]);
        }
    }
}

static void calc_handle_click(window_t* win, int rel_x, int rel_y) {
    if (!win || !win->user_data) return;
    calc_state_t* state = (calc_state_t*)win->user_data;

    int content_y = rel_y - 24;
    if (content_y < 48) return;

    int r = (content_y - 48) / 38;
    int c = (rel_x - 10) / 52;

    if (r < 0 || r >= 4 || c < 0 || c >= 4) return;

    const char* keys[4][4] = {
        {"7", "8", "9", "/"},
        {"4", "5", "6", "*"},
        {"1", "2", "3", "-"},
        {"C", "0", "=", "+"}
    };

    const char* key = keys[r][c];

    if (key[0] >= '0' && key[0] <= '9') {
        if (state->clear_on_next || strcmp(state->display, "0") == 0) {
            state->display[0] = key[0];
            state->display[1] = '\0';
            state->clear_on_next = 0;
        } else if (strlen(state->display) < sizeof(state->display) - 1) {
            int len = strlen(state->display);
            state->display[len] = key[0];
            state->display[len + 1] = '\0';
        }
    }
    else if (key[0] == 'C') {
        strcpy(state->display, "0");
        state->val1 = 0;
        state->val2 = 0;
        state->op = 0;
        state->clear_on_next = 0;
    }
    else if (key[0] == '+' || key[0] == '-' || key[0] == '*' || key[0] == '/') {
        state->val1 = 0;
        for (int i = 0; state->display[i]; i++) {
            if (state->display[i] >= '0' && state->display[i] <= '9') {
                state->val1 = state->val1 * 10 + (state->display[i] - '0');
            }
        }
        state->op = key[0];
        state->clear_on_next = 1;
    }
    else if (key[0] == '=') {
        if (state->op) {
            state->val2 = 0;
            for (int i = 0; state->display[i]; i++) {
                if (state->display[i] >= '0' && state->display[i] <= '9') {
                    state->val2 = state->val2 * 10 + (state->display[i] - '0');
                }
            }
            long res = 0;
            if (state->op == '+') {
                res = state->val1 + state->val2;
            } else if (state->op == '-') {
                res = state->val1 - state->val2;
            } else if (state->op == '*') {
                res = state->val1 * state->val2;
            } else if (state->op == '/') {
                res = (state->val2 != 0) ? (state->val1 / state->val2) : 0;
            }

            sprintf(state->display, "%ld", res);
            state->op = 0;
            state->clear_on_next = 1;
        }
    }

    calc_redraw(win);
}

void calc_open(void) {
    window_t* win = wm_create_window(250, 140, 235, 235, "Calculator");
    if (!win) return;

    calc_state_t* state = (calc_state_t*)kmalloc(sizeof(calc_state_t));
    if (!state) return;
    memset(state, 0, sizeof(calc_state_t));
    strcpy(state->display, "0");

    win->user_data = state;
    win->render_content = calc_redraw;
    win->on_click = calc_handle_click;
    taskbar_add_button(win->id, "Calculator");
    calc_redraw(win);
}
