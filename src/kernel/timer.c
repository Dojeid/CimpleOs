#include "timer.h"
#include "io.h"

volatile uint32_t timer_ticks = 0;

void timer_handler() {
    timer_ticks++;
    outb(0x20, 0x20); // Send EOI
}

void timer_init(uint32_t frequency) {
    // PIT frequency is 1193180 Hz
    uint32_t divisor = 1193180 / frequency;
    
    // Send command byte
    outb(0x43, 0x36);
    
    // Send frequency divisor
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}

uint32_t timer_get_ticks() {
    return timer_ticks;
}

void timer_wait(uint32_t ticks) {
    uint32_t start = timer_ticks;
    while (timer_ticks < start + ticks);
}
