#ifndef TASKBAR_H
#define TASKBAR_H

#include <stdint.h>
#include "window_manager.h"

#define TASKBAR_HEIGHT 30
#define MAX_TASKBAR_BUTTONS 8

// Launcher button (permanent, not tied to windows)
typedef struct {
    int x;
    int width;
    char label[32];
    int enabled;
} launcher_button_t;

// Taskbar button
typedef struct {
    int window_id;
    char label[32];
    int x, width;  // Position and size
} taskbar_button_t;

// Taskbar state
typedef struct {
    taskbar_button_t buttons[MAX_TASKBAR_BUTTONS];
    int button_count;
    int y_position;
} taskbar_t;

// Initialize taskbar
void taskbar_init();

// Add button for a window
void taskbar_add_button(int window_id, const char* label);

// Remove button for a window
void taskbar_remove_button(int window_id);

// Render taskbar
void taskbar_render();

// Handle click on taskbar
void taskbar_handle_click(int x, int y);

// Get taskbar state
taskbar_t* taskbar_get_state();

#endif
