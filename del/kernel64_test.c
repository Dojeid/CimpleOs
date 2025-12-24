#include <stdint.h>
#include <stddef.h>

// Minimal 64-bit kernel for testing

extern void idt64_init(void);

// VGA text mode buffer (works in 64-bit)
static uint16_t* const vga_buffer = (uint16_t*)0xB8000;

static void print_string(const char* str) {
    static size_t x = 0;
    static size_t y = 0;
    
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            x = 0;
            y++;
            continue;
        }
        
        const size_t index = y * 80 + x;
        vga_buffer[index] = (uint16_t)str[i] | (0x0F << 8);  // White on black
        
        x++;
        if (x >= 80) {
            x = 0;
            y++;
        }
    }
}

// Kernel main (called from boot.asm with multiboot info)
void kmain(void* multiboot_info) {
    // Clear screen
    for (size_t i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = 0x0F20;  // Space with white on black
    }
    
    // Print success message
    print_string("CimpleOS 64-bit\n");
    print_string("Long mode active!\n");
    print_string("Initializing IDT...\n");
    
    // Initialize interrupts
    idt64_init();
    
    print_string("IDT initialized\n");
    print_string("Kernel running\n");
    
    // Halt
    while (1) {
        asm volatile("hlt");
    }
}
