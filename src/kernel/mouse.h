#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

extern int mouse_x;
extern int mouse_y;

void init_mouse();
void mouse_handler();
int mouse_button_left();
int mouse_button_pressed();
int mouse_button_released();

#endif
