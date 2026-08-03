#ifndef GUI_CONTEXT_MENU_H
#define GUI_CONTEXT_MENU_H

#include <stdint.h>

#define CONTEXT_MENU_MAX_ITEMS 10

typedef struct {
    char label[48];
    void (*on_select)(void);
    uint32_t color;
    int separator;
} context_menu_item_t;

typedef struct {
    int visible;
    int x, y;
    int width, height;
    context_menu_item_t items[CONTEXT_MENU_MAX_ITEMS];
    int item_count;
    int hovered_item;
} context_menu_t;

void context_menu_show(int x, int y);
void context_menu_hide(void);
void context_menu_add_item(const char* label, void (*on_select)(void));
void context_menu_add_separator(void);
void context_menu_clear(void);
void context_menu_render(void);
int context_menu_handle_click(int x, int y);
int context_menu_handle_hover(int x, int y);
int context_menu_is_visible(void);
void context_menu_setup_desktop(void);
void context_menu_setup_window(int win_id);

#endif
