#include "paint.h"
#include "../../drivers/video/graphics.h"
#include "../../gui/window_manager.h"
#include "../../mm/heap.h"
#include "../../lib/string.h"
#include "../../gui/taskbar.h"
#include <stdint.h>
#include <stddef.h>

typedef enum {
    TOOL_PENCIL,
    TOOL_ERASER,
    TOOL_LINE,
    TOOL_RECT,
    TOOL_CIRCLE,
    TOOL_FILL
} paint_tool_t;

typedef struct {
    paint_tool_t current_tool;
    uint32_t current_color;
    int brush_size;
    int drawing;
    int start_x, start_y;
    uint32_t canvas[600 * 368];
    int canvas_w, canvas_h;
} paint_state_t;

static paint_state_t* g_paint_state = NULL;
static window_t* g_paint_win = NULL;

static const uint32_t PALETTE[16] = {
    0x000000, 0xFFFFFF, 0xEF4444, 0xF97316,
    0xEAB308, 0x22C55E, 0x06B6D4, 0x3B82F6,
    0x8B5CF6, 0xEC4899, 0x78716C, 0x9CA3AF,
    0x1E3A5F, 0x14532D, 0xFEF3C7, 0x0F172A
};

static const char* TOOL_LABELS[] = { "P", "E", "L", "R", "C", "F" };

static void paint_flood_fill(paint_state_t* ps, int x, int y, uint32_t new_color) {
    if (x < 0 || x >= ps->canvas_w || y < 0 || y >= ps->canvas_h) return;
    
    uint32_t old_color = ps->canvas[y * ps->canvas_w + x];
    if (old_color == new_color) return;
    
    static int qx[8192], qy[8192];
    int head = 0, tail = 0;
    qx[tail] = x; 
    qy[tail] = y; 
    tail++;
    
    while (head != tail) {
        int cx = qx[head];
        int cy = qy[head];
        head = (head + 1) % 8192;
        
        if (cx < 0 || cx >= ps->canvas_w || cy < 0 || cy >= ps->canvas_h) continue;
        if (ps->canvas[cy * ps->canvas_w + cx] != old_color) continue;
        
        ps->canvas[cy * ps->canvas_w + cx] = new_color;
        
        qx[tail % 8192] = cx + 1; qy[tail % 8192] = cy; tail = (tail + 1) % 8192;
        qx[tail % 8192] = cx - 1; qy[tail % 8192] = cy; tail = (tail + 1) % 8192;
        qx[tail % 8192] = cx;     qy[tail % 8192] = cy + 1; tail = (tail + 1) % 8192;
        qx[tail % 8192] = cx;     qy[tail % 8192] = cy - 1; tail = (tail + 1) % 8192;
    }
}

static void draw_brush(paint_state_t* ps, int cx, int cy, uint32_t color) {
    int r = ps->brush_size / 2;
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            int x = cx + dx;
            int y = cy + dy;
            if (x >= 0 && x < ps->canvas_w && y >= 0 && y < ps->canvas_h) {
                if (dx*dx + dy*dy <= r*r) {
                    ps->canvas[y * ps->canvas_w + x] = color;
                }
            }
        }
    }
}

static void paint_redraw(window_t* win) {
    if (!g_paint_state) return;
    
    // Draw toolbar background
    draw_rect(win->x, win->y, win->width, 32, 0x1E293B);
    
    // Draw tool buttons
    for (int i = 0; i < 6; i++) {
        int bx = win->x + 10 + i * 30;
        int by = win->y + 4;
        uint32_t bg = ((int)g_paint_state->current_tool == i) ? 0x38BDF8 : 0x475569;
        draw_rect(bx, by, 24, 24, bg);
        draw_string(bx + 8, by + 4, 0xFFFFFF, TOOL_LABELS[i]);
    }
    
    // Draw palette
    for (int i = 0; i < 16; i++) {
        int px = win->x + win->width - 16 * 18 + i * 18 - 10;
        int py = win->y + 8;
        draw_rect(px, py, 16, 16, PALETTE[i]);
        if (g_paint_state->current_color == PALETTE[i]) {
            // highlight selection
            draw_rect(px + 4, py + 4, 8, 8, 0xFFFFFF ^ PALETTE[i]);
        }
    }
    
    // Fast clipped row blit of canvas pixels
    extern uint32_t* back_buffer;
    extern int screen_w, screen_h;

    int cw = g_paint_state->canvas_w;
    int ch = g_paint_state->canvas_h;
    int base_x = win->x;
    int base_y = win->y + 32;

    for (int y = 0; y < ch; y++) {
        int dst_y = base_y + y;
        if (dst_y < 0 || dst_y >= screen_h) continue;

        uint32_t* src_row = &g_paint_state->canvas[y * cw];
        uint32_t* dst_row = &back_buffer[dst_y * screen_w];

        int start_x = (base_x < 0) ? -base_x : 0;
        int end_x = (base_x + cw > screen_w) ? (screen_w - base_x) : cw;

        for (int x = start_x; x < end_x; x++) {
            dst_row[base_x + x] = src_row[x];
        }
    }
}

static void paint_handle_click_internal(window_t* win, int rel_x, int rel_y, int button) {
    if (!g_paint_state) return;
    
    if (rel_y < 32) {
        // Toolbar click
        for (int i = 0; i < 6; i++) {
            int bx = 10 + i * 30;
            if (rel_x >= bx && rel_x < bx + 24 && rel_y >= 4 && rel_y < 28) {
                g_paint_state->current_tool = (paint_tool_t)i;
                return;
            }
        }
        for (int i = 0; i < 16; i++) {
            int px = win->width - 16 * 18 + i * 18 - 10;
            if (rel_x >= px && rel_x < px + 16 && rel_y >= 8 && rel_y < 24) {
                g_paint_state->current_color = PALETTE[i];
                return;
            }
        }
    } else {
        // Canvas click
        int cx = rel_x;
        int cy = rel_y - 32;
        
        if (cx >= 0 && cx < g_paint_state->canvas_w && cy >= 0 && cy < g_paint_state->canvas_h) {
            if (g_paint_state->current_tool == TOOL_PENCIL) {
                draw_brush(g_paint_state, cx, cy, g_paint_state->current_color);
            } else if (g_paint_state->current_tool == TOOL_ERASER) {
                draw_brush(g_paint_state, cx, cy, 0xFFFFFF);
            } else if (g_paint_state->current_tool == TOOL_FILL) {
                paint_flood_fill(g_paint_state, cx, cy, g_paint_state->current_color);
            } else {
                g_paint_state->start_x = cx;
                g_paint_state->start_y = cy;
            }
            g_paint_state->drawing = 1;
        }
    }
}

static void paint_on_click(window_t* win, int rel_x, int rel_y) {
    paint_handle_click_internal(win, rel_x, rel_y, 1);
}

void paint_app_open(void) {
    if (g_paint_win) return;
    
    g_paint_state = (paint_state_t*)malloc(sizeof(paint_state_t));
    if (!g_paint_state) return;
    
    g_paint_state->current_tool = TOOL_PENCIL;
    g_paint_state->current_color = 0x000000;
    g_paint_state->brush_size = 5;
    g_paint_state->drawing = 0;
    g_paint_state->canvas_w = 600;
    g_paint_state->canvas_h = 368;
    
    for (int i = 0; i < 600 * 368; i++) {
        g_paint_state->canvas[i] = 0xFFFFFF;
    }
    
    g_paint_win = wm_create_window(100, 100, 600, 400, "Paint");
    if (g_paint_win) {
        g_paint_win->render_content = paint_redraw;
        g_paint_win->on_click = paint_on_click;
        g_paint_win->user_data = g_paint_state;
        taskbar_add_button(g_paint_win->id, "Paint");
    }
}

void paint_app_handle_mouse(int x, int y, int button) {
    // A more generic handler, can be used for dragging
    if (!g_paint_state || !g_paint_win) return;
    
    if (button == 0) {
        g_paint_state->drawing = 0;
        return;
    }
    
    if (g_paint_state->drawing) {
        int rel_x = x - g_paint_win->x;
        int rel_y = y - g_paint_win->y;
        if (rel_y >= 32) {
            int cx = rel_x;
            int cy = rel_y - 32;
            if (g_paint_state->current_tool == TOOL_PENCIL) {
                draw_brush(g_paint_state, cx, cy, g_paint_state->current_color);
            } else if (g_paint_state->current_tool == TOOL_ERASER) {
                draw_brush(g_paint_state, cx, cy, 0xFFFFFF);
            }
        }
    }
}
