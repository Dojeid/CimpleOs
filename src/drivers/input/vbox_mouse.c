#include "vbox_mouse.h"
#include "drivers/bus/pci.h"
#include "mm/vmm.h"
#include "lib/io.h"
#include "lib/string.h"
#include "drivers/video/vga.h"

#define VBOX_VENDOR_ID 0x80EE
#define VBOX_DEVICE_ID 0xCAFE

#define VMMDEV_REQUEST_HEADER_SIZE 24
#define VMMDEVREQ_SET_MOUSE_STATUS 101
#define VMMDEVREQ_GET_MOUSE_STATUS 102

// VirtualBox VMMDev Mouse Feature Flags:
#define VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE     (1 << 0)
#define VMMDEV_MOUSE_GUEST_IS_VISIBLE        (1 << 1)
#define VMMDEV_MOUSE_GUEST_NEEDS_HOST_CURSOR (1 << 4)

typedef struct __attribute__((packed)) {
    uint32_t size;
    uint32_t version;
    uint32_t request_type;
    int32_t  rc;
    uint32_t reserved1;
    uint32_t reserved2;
} vbox_header_t;

typedef struct __attribute__((packed)) {
    vbox_header_t header;
    uint32_t features;
    int32_t  x;
    int32_t  y;
} vbox_mouse_req_t;

static uint16_t vbox_io_port = 0;
static int vbox_active = 0;
static volatile vbox_mouse_req_t mouse_req;

static uint32_t get_phys_req_addr(void) {
    uint64_t phys = vmm_translate((uint64_t)&mouse_req);
    return (uint32_t)(phys ? phys : (uintptr_t)&mouse_req);
}

int vbox_mouse_init(void) {
    struct pci_device dev;
    if (!pci_find_by_id(VBOX_VENDOR_ID, VBOX_DEVICE_ID, &dev)) {
        vga_print("[VBox] VMMDev PCI Device not present.\n");
        vbox_active = 0;
        return 0;
    }

    vbox_io_port = (uint16_t)(dev.bar0 & 0xFFFC);
    if (vbox_io_port == 0) {
        vbox_io_port = 0x5044; // Standard VirtualBox VMMDev I/O port fallback
    }

    // Enable Seamless Guest Absolute Pointer & Host Auto-Release
    memset((void*)&mouse_req, 0, sizeof(mouse_req));
    mouse_req.header.size = sizeof(mouse_req);
    mouse_req.header.version = 0x00010001; // VMMDEV_REQUEST_HEADER_VERSION
    mouse_req.header.request_type = VMMDEVREQ_SET_MOUSE_STATUS;
    mouse_req.features = VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE | 
                         VMMDEV_MOUSE_GUEST_IS_VISIBLE | 
                         VMMDEV_MOUSE_GUEST_NEEDS_HOST_CURSOR;

    // Send request to VirtualBox VMMDev port using physical address
    asm volatile("": : :"memory"); // memory barrier
    outl(vbox_io_port, get_phys_req_addr());
    asm volatile("": : :"memory");

    vbox_active = 1;
    vga_print("[VBox] VirtualBox Guest Integration Active! Seamless Mouse Release ENABLED.\n");
    return 1;
}

int vbox_mouse_poll(int* out_x, int* out_y, uint8_t* out_buttons) {
    if (!vbox_active || !out_x || !out_y) return 0;

    memset((void*)&mouse_req, 0, sizeof(mouse_req));
    mouse_req.header.size = sizeof(mouse_req);
    mouse_req.header.version = 0x00010001;
    mouse_req.header.request_type = VMMDEVREQ_GET_MOUSE_STATUS;
    mouse_req.x = -1;
    mouse_req.y = -1;

    asm volatile("": : :"memory"); // Ensure struct is written before outl
    outl(vbox_io_port, get_phys_req_addr());
    asm volatile("": : :"memory"); // Ensure struct is re-read from memory after outl // Ensure struct is re-read from memory after outl

    // Translate VirtualBox absolute coordinates (0 .. 65535)
    if (mouse_req.x >= 0 && mouse_req.y >= 0 && mouse_req.x <= 65535 && mouse_req.y <= 65535) {
        extern int screen_w, screen_h;
        int px = (mouse_req.x * screen_w) / 65535;
        int py = (mouse_req.y * screen_h) / 65535;
        
        if (px >= screen_w) px = screen_w - 1;
        if (py >= screen_h) py = screen_h - 1;
        
        *out_x = px;
        *out_y = py;
        if (out_buttons) *out_buttons = 0; // Handled by IRQ12 button mask
        return 1;
    }

    return 0;
}

int vbox_mouse_is_active(void) {
    return vbox_active;
}
