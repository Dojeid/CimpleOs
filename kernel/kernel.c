#include "vga.h"

void kmain(void) {
    terminal_initialize();
    terminal_writestring("\nWelcome to CimpleOS.\n");
    terminal_writestring("CimpleOS kernel reporting for duty!");
    terminal_writestring("\nCimple... Makes Life Simple...\n");
}