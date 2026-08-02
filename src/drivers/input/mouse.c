#include <stdint.h>
#include <stddef.h>
#include "kernel/idt.h"
#include "lib/io.h"

// --- MOUSE STATE ---
// --- MOUSE STATE ---
// SECURITY: All ISR-modified variables marked volatile to prevent compiler caching
volatile int mouse_x = 400;
volatile int mouse_y = 300;
volatile uint8_t mouse_cycle = 0;
int8_t mouse_byte[3];
volatile uint8_t mouse_left_btn = 0;
volatile uint8_t mouse_click_latch = 0;
volatile uint8_t mouse_release_latch = 0;

extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);

void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) { if ((inb(0x64) & 1) == 1) return; }
    } else {
        while (timeout--) { if ((inb(0x64) & 2) == 0) return; }
    }
}

void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, write);
}

uint8_t mouse_read() {
    mouse_wait(0);
    return inb(0x60);
}

#include "vbox_mouse.h"

void init_mouse() {
    uint8_t status;
    
    mouse_wait(1);
    outb(0x64, 0xA8); // Enable auxiliary mouse port
    
    mouse_wait(1);
    outb(0x64, 0x20); // Read Command Byte
    mouse_wait(0);
    status = (inb(0x60) | 0x02) & ~0x20; // Enable IRQ 12 (bit 1) and CLEAR Disable Mouse (bit 5)
    mouse_wait(1);
    outb(0x64, 0x60); // Write Command Byte
    mouse_wait(1);
    outb(0x60, status);
    
    mouse_write(0xF6); // Set Defaults
    mouse_read();
    
    mouse_write(0xF4); // Enable Packet Streaming
    mouse_read();

    // Initialize VirtualBox Guest VMMDev Absolute Mouse
    vbox_mouse_init();
}

void mouse_handler() {
    uint8_t status = inb(0x64);
    if ((status & 0x01) == 0 || (status & 0x20) == 0) {
        return;
    }
    
    uint8_t mouse_in = inb(0x60);
    
    // Accumulate PS/2 3-byte packet
    mouse_byte[mouse_cycle] = mouse_in;
    mouse_cycle++;
    
    // If packet index is 1 (byte 0 flags), verify bit 3 alignment flag (must be 1 for PS/2 mouse packet header)
    if (mouse_cycle == 1 && !(mouse_byte[0] & 0x08)) {
        mouse_cycle = 0;
        return;
    }

    if (mouse_cycle == 3) {
        mouse_cycle = 0;
        
        uint8_t flags = mouse_byte[0];
        int16_t x_rel = (int16_t)(int8_t)mouse_byte[1];
        int16_t y_rel = (int16_t)(int8_t)mouse_byte[2];
        
        if (flags & 0x10) x_rel = (int16_t)((uint16_t)x_rel | 0xFF00); // Sign extend X
        if (flags & 0x20) y_rel = (int16_t)((uint16_t)y_rel | 0xFF00); // Sign extend Y
        
        extern int screen_w, screen_h;

        // Button state updated cleanly at end of 3-byte cycle
        uint8_t new_btn = (flags & 0x01);
        if (new_btn && !mouse_left_btn) mouse_click_latch = 1;
        if (!new_btn && mouse_left_btn) mouse_release_latch = 1;
        mouse_left_btn = new_btn;

        // Check VirtualBox VMMDev Absolute Mouse Integration
        int vbox_x = 0, vbox_y = 0;
        if (vbox_mouse_is_active() && vbox_mouse_poll(&vbox_x, &vbox_y, NULL)) {
            mouse_x = vbox_x;
            mouse_y = vbox_y;
        } else {
            // Standard PS/2 Relative Positioning
            if ((mouse_x <= 0 && x_rel < 0) || (mouse_x >= screen_w - 1 && x_rel > 0)) {
                x_rel = 0;
            }
            if ((mouse_y <= 0 && y_rel < 0) || (mouse_y >= screen_h - 1 && y_rel > 0)) {
                y_rel = 0;
            }

            // Ignore overflow packets and clamp per-packet deltas
            if ((flags & 0xC0) == 0) {
                if (x_rel > 45) x_rel = 45;
                if (x_rel < -45) x_rel = -45;
                if (y_rel > 45) y_rel = 45;
                if (y_rel < -45) y_rel = -45;

                mouse_x += x_rel;
                mouse_y -= y_rel;
            }
        }
        
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x >= screen_w) mouse_x = screen_w - 1;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y >= screen_h) mouse_y = screen_h - 1;
    }
}

int mouse_button_left() {
    return mouse_left_btn;
}

// Critical section with debounce
int mouse_button_pressed() {
    uint64_t flags;
    int pressed = 0;
    
    static uint32_t last_press_time = 0;
    extern volatile uint32_t timer_ticks;
    
    // Save RFLAGS correctly (pushfq; popq; cli)
    asm volatile("pushfq; pop %0; cli" : "=r"(flags) :: "memory");
    
    if (mouse_click_latch) {
        mouse_click_latch = 0;
        if (timer_ticks - last_press_time >= 10) {
            pressed = 1;
            last_press_time = timer_ticks;
        }
    }
    
    if (flags & 0x200) asm volatile("sti");
    
    return pressed;
}

int mouse_button_released() {
    uint64_t flags;
    int released = 0;
    
    asm volatile("pushfq; pop %0; cli" : "=r"(flags) :: "memory");
    
    if (mouse_release_latch) {
        mouse_release_latch = 0;
        released = 1;
    }
    
    if (flags & 0x200) asm volatile("sti");
    
    return released;
}

void mouse_update_vbox() {
    // VBox polling is already performed in mouse_handler on IRQ12; no double polling needed
}
