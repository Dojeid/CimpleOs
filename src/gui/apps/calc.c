#include "gui/apps/calc.h"
#include "gui/window_manager.h"
#include "gui/taskbar.h"
#include "drivers/video/graphics.h"
#include "lib/string.h"
#include "lib/printf.h"

static char calc_display[32] = "0";

static void calc_redraw(window_t* win) {
    if (!win) return;
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x1E1E2E);

    // Calculator LCD Display
    draw_rect(win->x + 10, win->y + 32, win->width - 20, 30, 0x0F0F17);
    draw_rect(win->x + 10, win->y + 32, win->width - 20, 1, 0x38BDF8);

    // Display alignment
    int text_len = strlen(calc_display);
    int start_x = win->x + win->width - 25 - (text_len * 8);
    if (start_x < win->x + 16) start_x = win->x + 16;
    draw_string(start_x, win->y + 40, 0x38BDF8, calc_display);

    // Keypad Buttons Grid
    const char* keys[4][4] = {
        {"7", "8", "9", "/"},
        {"4", "5", "6", "*"},
        {"1", "2", "3", "-"},
        {"C", "0", "=", "+"}
    };

    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int bx = win->x + 12 + c * 52;
            int by = win->y + 70 + r * 38;
            uint32_t bg = (c == 3) ? 0xF59E0B : ((r == 3 && c == 0) ? 0xEF4444 : ((r == 3 && c == 2) ? 0x10B981 : 0x334155));
            draw_rect(bx, by, 46, 32, bg);
            draw_string(bx + 18, by + 10, 0xFFFFFF, keys[r][c]);
        }
    }
}

void calc_open(void) {
    window_t* win = wm_create_window(250, 140, 235, 235, "Calculator");
    if (win) {
        win->render_content = calc_redraw;
        taskbar_add_button(win->id, "Calculator");
        calc_redraw(win);
    }
}
