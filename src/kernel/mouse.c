#include <stdint.h>
#include "idt.h"
#include "io.h"

// --- MOUSE STATE ---
int mouse_x = 400; // Start in center
int mouse_y = 300;
uint8_t mouse_cycle = 0;   // 0, 1, or 2 (We need 3 bytes for a packet)
int8_t mouse_byte[3];      // The data packet
uint8_t mouse_left_btn = 0;

extern void outb(uint16_t port, uint8_t val);
extern uint8_t inb(uint16_t port);

// Helper to wait for the mouse to be ready
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

void init_mouse() {
    uint8_t status;

    // Enable Aux Device
    mouse_wait(1);
    outb(0x64, 0xA8);

    // Enable Interrupts
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    status = (inb(0x60) | 2);
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    // Default Settings
    mouse_write(0xF6);
    mouse_read();

    // Enable Data Reporting
    mouse_write(0xF4);
    mouse_read();
}

// THE MOUSE INTERRUPT HANDLER (IRQ 12)
void mouse_handler() {
    uint8_t status = inb(0x64);
    if ((status & 0x01) == 0) {
        // No data? Just exit.
        return;
    }
    if ((status & 0x20) == 0) {
        // Data is not from mouse? Exit. (Shouldn't happen on IRQ 12)
        return;
    }

    uint8_t mouse_in = inb(0x60);

    // Process Packet
    mouse_byte[mouse_cycle] = mouse_in;
    mouse_cycle++;

    if (mouse_cycle == 3) { // Packet complete!
        mouse_cycle = 0;

        // Parse Data
        uint8_t flags = mouse_byte[0];
        int8_t x_rel = (int8_t)mouse_byte[1];
        int8_t y_rel = (int8_t)mouse_byte[2];

        // Update Position
        mouse_x += x_rel;
        mouse_y -= y_rel; // Y is inverted on PS/2

        // Clamp to screen (Assuming 1024x768)
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_x > 1023) mouse_x = 1023;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_y > 767) mouse_y = 767;

        // Click State
        if (flags & 0x01) mouse_left_btn = 1;
        else mouse_left_btn = 0;
    }
    
    outb(0xA0, 0x20); // EOI to Slave PIC
    outb(0x20, 0x20); // EOI to Master PIC
}