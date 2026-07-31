#include "gui/apps/calc.h"
#include "gui/window_manager.h"
#include "drivers/video/graphics.h"
#include "lib/printf.h"

static void calc_redraw(window_t* win) {
    if (!win) return;
    draw_rect(win->x + 2, win->y + 24, win->width - 4, win->height - 26, 0x222222);

    // Calculator LCD Display
    draw_rect(win->x + 10, win->y + 32, win->width - 20, 28, 0x000000);
    draw_string(win->x + win->width - 70, win->y + 40, 0x00FF00, "1337.00");

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
            uint32_t bg = (c == 3) ? 0xFF9500 : ((r == 3 && c == 0) ? 0xFF3B30 : 0x333333);
            draw_rect(bx, by, 46, 32, bg);
            draw_string(bx + 18, by + 10, 0xFFFFFF, keys[r][c]);
        }
    }
}

void calc_open(void) {
    window_t* win = wm_create_window(250, 140, 235, 235, "Calculator");
    if (win) {
        win->render_content = calc_redraw;
        calc_redraw(win);
    }
}
