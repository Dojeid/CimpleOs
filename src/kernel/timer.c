#include "timer.h"
#include "lib/io.h"

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
    // BUG FIX #7: Add overflow protection for timer_ticks
    uint32_t start = timer_ticks;
    uint32_t target = start + ticks;
    
    // Handle overflow: if target wraps around, wait until we pass start or wrap
    while (1) {
        // Check if we've passed the target (considering overflow)
        if (timer_ticks >= target) break;
        
        // If target wrapped (less than start), wait for overflow or pass
        if (target < start) {
            if (timer_ticks >= start || timer_ticks < target) break;
        } else {
            // Normal case - target is greater than start
            if (timer_ticks >= target) break;
        }
    }
}
