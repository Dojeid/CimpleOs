#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// Keyboard handler (called from IRQ1)
void keyboard_handler(void);

// Keyboard state exported for other modules
extern char terminal_buffer[];
extern int term_idx;
extern volatile int irq_count;

#endif
