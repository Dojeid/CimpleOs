#ifndef CURSOR_H
#define CURSOR_H

#include <stdint.h>

typedef enum {
    CURSOR_ARROW,
    CURSOR_HAND,
    CURSOR_RESIZE,
    CURSOR_TEXT
} cursor_type_t;

typedef struct {
    int x;
    int y;
    int visible;
    cursor_type_t type;
} cursor_t;

void cursor_init();
void cursor_set_position(int x, int y);
void cursor_get_position(int* x, int* y);
void cursor_set_visible(int visible);
void cursor_set_type(cursor_type_t type);
void cursor_set_screen_bounds(int w, int h);
void cursor_render();

#endif
