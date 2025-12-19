#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// Timer state
extern volatile uint32_t timer_ticks;

// Initialize PIT timer
void timer_init(uint32_t frequency);

// Get current tick count
uint32_t timer_get_ticks();

// Sleep for specified ticks
void timer_wait(uint32_t ticks);

#endif
