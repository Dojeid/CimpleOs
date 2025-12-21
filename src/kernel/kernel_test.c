#include <stdint.h>
#include <stddef.h>
#include "multiboot.h"
#include "vga.h"

// --- MINIMAL TEST KERNEL ---
// If this works, we know GRUB loads our kernel
// If this fails, there's a bootloader issue

void kmain(void* mb_info_ptr) {
    // ABSOLUTE MINIMUM - just write directly to VGA memory
    uint16_t* vga = (uint16_t*)0xB8000;
    
    // Write "CimpleOS!" in white on black
    const char* msg = "CimpleOS!";
    for (int i = 0; msg[i] != '\0'; i++) {
        vga[i] = (uint16_t)msg[i] | 0x0F00;
    }
    
    // Halt forever
    while(1) {
        asm("hlt");
    }
}
