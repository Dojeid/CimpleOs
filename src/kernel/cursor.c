#include "cursor.h"
#include "graphics.h"

static cursor_t cursor = {0, 0, 1, CURSOR_ARROW};

// Arrow cursor bitmap (16x16)
static const uint8_t cursor_arrow[16][16] = {
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,1,1,1,0,0,0,0,0,0},
    {1,2,2,1,2,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,1,2,2,1,0,0,0,0,0,0,0,0},
    {1,1,0,0,1,2,2,1,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,2,2,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,1,2,2,1,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0}
};

void cursor_init() {
    cursor.x = 320;
    cursor.y = 240;
    cursor.visible = 1;
    cursor.type = CURSOR_ARROW;
}

void cursor_set_position(int x, int y) {
    cursor.x = x;
    cursor.y = y;
}

void cursor_get_position(int* x, int* y) {
    if (x) *x = cursor.x;
    if (y) *y = cursor.y;
}

void cursor_set_visible(int visible) {
    cursor.visible = visible;
}

void cursor_set_type(cursor_type_t type) {
    cursor.type = type;
}

void cursor_render() {
    if (!cursor.visible) return;
    
    // Render based on type
    switch (cursor.type) {
        case CURSOR_ARROW:
            // Draw arrow cursor
            for (int py = 0; py < 16; py++) {
                for (int px = 0; px < 16; px++) {
                    uint8_t pixel = cursor_arrow[py][px];
                    
                    if (pixel == 0) continue;  // Transparent
                    
                    int screen_x = cursor.x + px;
                    int screen_y = cursor.y + py;
                    
                    uint32_t color;
                    if (pixel == 1) {
                        color = 0x000000;  // Black outline
                    } else {
                        color = 0xFFFFFF;  // White fill
                    }
                    
                    put_pixel(screen_x, screen_y, color);
                }
            }
            break;
            
        case CURSOR_HAND:
            // Simple hand cursor (just a filled square for now)
            draw_rect(cursor.x, cursor.y, 12, 12, 0xFFFF00);
            break;
            
        case CURSOR_RESIZE:
            // Resize cursor (cross)
            draw_rect(cursor.x, cursor.y - 5, 1, 11, 0xFFFFFF);
            draw_rect(cursor.x - 5, cursor.y, 11, 1, 0xFFFFFF);
            break;
            
        case CURSOR_TEXT:
            // I-beam cursor
            draw_rect(cursor.x, cursor.y, 1, 14, 0xFFFFFF);
            draw_rect(cursor.x - 2, cursor.y, 5, 1, 0xFFFFFF);
            draw_rect(cursor.x - 2, cursor.y + 13, 5, 1, 0xFFFFFF);
            break;
    }
}
