#ifndef CURSOR_H
#define CURSOR_H

#include <stdint.h>

// Cursor types
typedef enum {
    CURSOR_ARROW,
    CURSOR_HAND,
    CURSOR_RESIZE,
    CURSOR_TEXT
} cursor_type_t;

// Cursor state
typedef struct {
    int x, y;
    int visible;
    cursor_type_t type;
} cursor_t;

// Initialize cursor system
void cursor_init();

// Set cursor position
void cursor_set_position(int x, int y);

// Get cursor position
void cursor_get_position(int* x, int* y);

// Set cursor visibility
void cursor_set_visible(int visible);

// Set cursor type
void cursor_set_type(cursor_type_t type);

// Render cursor (call last, on top of everything)
void cursor_render();

#endif
