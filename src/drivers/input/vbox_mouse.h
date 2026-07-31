#ifndef VBOX_MOUSE_H
#define VBOX_MOUSE_H

#include <stdint.h>

int vbox_mouse_init(void);
int vbox_mouse_poll(int* out_x, int* out_y, uint8_t* out_buttons);
int vbox_mouse_is_active(void);

#endif // VBOX_MOUSE_H
