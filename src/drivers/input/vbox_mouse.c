#include "vbox_mouse.h"
#include "drivers/bus/pci.h"
#include "lib/io.h"
#include "lib/string.h"
#include "drivers/video/vga.h"

#define VBOX_VENDOR_ID 0x80EE
#define VBOX_DEVICE_ID 0xCAFE

#define VMMDEV_REQUEST_HEADER_SIZE 24
#define VMMDEVREQ_SET_MOUSE_STATUS 101
#define VMMDEVREQ_GET_MOUSE_STATUS 102

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
static vbox_mouse_req_t mouse_req;

int vbox_mouse_init(void) {
    struct pci_device dev;
    if (!pci_find_by_id(VBOX_VENDOR_ID, VBOX_DEVICE_ID, &dev)) {
        vga_print("[VBox] VMMDev PCI Device not present.\n");
        vbox_active = 0;
        return 0;
    }

    vbox_io_port = (uint16_t)(dev.bar0 & 0xFFFC);
    if (vbox_io_port == 0) {
        vbox_io_port = 0x5040; // Default VirtualBox VMMDev port fallback
    }

    // Enable Guest Absolute Pointer Feature (0x01 | 0x02 | 0x04)
    memset(&mouse_req, 0, sizeof(mouse_req));
    mouse_req.header.size = sizeof(mouse_req);
    mouse_req.header.version = 0x00010001; // VMMDEV_REQUEST_HEADER_VERSION
    mouse_req.header.request_type = VMMDEVREQ_SET_MOUSE_STATUS;
    mouse_req.features = (1 << 0) | (1 << 4); // VMMDEV_MOUSE_GUEST_CAN_ABSOLUTE | VMMDEV_MOUSE_GUEST_NEEDS_HOST_CURSOR

    // Send request to VirtualBox VMMDev port
    outl(vbox_io_port, (uint32_t)(uintptr_t)&mouse_req);

    vbox_active = 1;
    vga_print("[VBox] VirtualBox Guest Integration Active! Mouse Integration UNGREYED.\n");
    return 1;
}

int vbox_mouse_poll(int* out_x, int* out_y, uint8_t* out_buttons) {
    if (!vbox_active || !out_x || !out_y) return 0;

    memset(&mouse_req, 0, sizeof(mouse_req));
    mouse_req.header.size = sizeof(mouse_req);
    mouse_req.header.version = 0x00010001;
    mouse_req.header.request_type = VMMDEVREQ_GET_MOUSE_STATUS;

    outl(vbox_io_port, (uint32_t)(uintptr_t)&mouse_req);

    if (mouse_req.x >= 0 && mouse_req.y >= 0) {
        extern int screen_w, screen_h;
        *out_x = (mouse_req.x * screen_w) / 65535;
        *out_y = (mouse_req.y * screen_h) / 65535;
        if (out_buttons) *out_buttons = 0; // Handled by IRQ12 button mask
        return 1;
    }

    return 0;
}

int vbox_mouse_is_active(void) {
    return vbox_active;
}
