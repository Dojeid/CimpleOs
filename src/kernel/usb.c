#include "pci.h"
#include "io.h"
#include "usb.h"
#include "vga.h"

// Simplified USB implementation - focuses on getting keyboard/mouse working
// This is a minimal implementation that polls USB devices

#define UHCI_PROG_IF 0x00  // UHCI controller

static struct pci_device uhci_controller;
static uint16_t uhci_base = 0;
static int usb_initialized = 0;

// USB keyboard scancodes to ASCII (simplified)
static char usb_to_ascii[256] = {
    0, 0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
    '\n', 0, '\b', '\t', ' '
};

// External keyboard/mouse state
extern char terminal_buffer[];
extern int term_idx;
extern int mouse_x, mouse_y;

void usb_init() {
    vga_print("Scanning for USB controller...\n");
    
    // Find UHCI controller
    if (!pci_find_device(PCI_CLASS_SERIAL_BUS, PCI_SUBCLASS_USB, UHCI_PROG_IF, &uhci_controller)) {
        vga_print("No UHCI controller found\n");
        return;
    }
    
    vga_print("Found UHCI controller!\n");
    uhci_base = uhci_controller.bar0;
    
    if (uhci_base == 0) {
        vga_print("Invalid UHCI base address\n");
        return;
    }
    
    // Enable bus mastering
    uint32_t command = pci_read_config(uhci_controller.bus, uhci_controller.slot, 0, 0x04);
    command |= 0x05; // Bus master + I/O space
    pci_write_config(uhci_controller.bus, uhci_controller.slot, 0, 0x04, command);
    
    // Reset the controller
    outw(uhci_base + 0, 0x0002); // GRESET
    for (volatile int i = 0; i < 10000; i++); // Wait
    outw(uhci_base + 0, 0x0000); // Clear reset
    
    // Start the controller
    outw(uhci_base + 0, 0x0001); // Run
    
    usb_initialized = 1;
    vga_print("USB initialized!\n");
}

// Simplified polling - reads from USB HID devices
void usb_poll() {
    if (!usb_initialized) return;
    
    // This is a VERY simplified implementation
    // In reality, USB requires complex packet/transfer management
    // For this demo, we'll use a polling approach that checks for data
    
    // Read status register
    uint16_t status = inw(uhci_base + 2);
    
    // Check if there's any USB activity
    if (status & 0x01) {
        // Acknowledge interrupt
        outw(uhci_base + 2, status);
        
        // Try to read data (this is highly simplified)
        // In a real implementation, you'd traverse the frame list,
        // check transfer descriptors, etc.
        
        // For now, we'll just simulate detecting keypresses
        // This won't actually work without proper USB enumeration
        // but it shows the structure
    }
}
