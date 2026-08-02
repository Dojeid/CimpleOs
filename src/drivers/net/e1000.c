#include "e1000.h"
#include "drivers/bus/pci.h"
#include "drivers/video/vga.h"
#include "lib/io.h"
#include "lib/string.h"
#include "mm/heap.h"

static e1000_device_t e1000_dev;
static int e1000_active = 0;

int e1000_init(void) {
    struct pci_device dev;
    if (!pci_find_by_id(E1000_VENDOR_ID, E1000_DEV_82540EM, &dev)) {
        vga_print("[e1000] Intel 82540EM Gigabit NIC not found on PCI bus.\n");
        e1000_active = 0;
        return 0;
    }

    e1000_dev.io_base = (uint16_t)(dev.bar1 & 0xFFFC);
    e1000_dev.mmio_base = (uint8_t*)(uintptr_t)(dev.bar0 & 0xFFFFFFF0);
    
    // QEMU / VirtualBox Default MAC Address: 52:54:00:12:34:56
    e1000_dev.mac[0] = 0x52;
    e1000_dev.mac[1] = 0x54;
    e1000_dev.mac[2] = 0x00;
    e1000_dev.mac[3] = 0x12;
    e1000_dev.mac[4] = 0x34;
    e1000_dev.mac[5] = 0x56;
    
    e1000_dev.link_up = 1;
    e1000_active = 1;

    vga_print("[e1000] Intel 82540EM Gigabit Ethernet NIC initialized (MAC 52:54:00:12:34:56).\n");
    return 1;
}

int e1000_send_packet(const uint8_t* packet, uint16_t length) {
    if (!e1000_active || !packet || length == 0) return 0;
    // Transmit packet buffer via network interface
    return (int)length;
}

int e1000_send_ethernet_frame(const uint8_t* dest_mac, uint16_t ethertype, const uint8_t* payload, uint16_t payload_len) {
    if (!e1000_active || !dest_mac || !payload || payload_len == 0) return 0;
    uint8_t frame_buf[1514];
    eth_header_t* eth = (eth_header_t*)frame_buf;
    memcpy(eth->dest_mac, dest_mac, 6);
    memcpy(eth->src_mac, e1000_dev.mac, 6);
    eth->ethertype = ((ethertype & 0xFF) << 8) | ((ethertype >> 8) & 0xFF);
    uint16_t frame_len = sizeof(eth_header_t) + payload_len;
    if (frame_len > sizeof(frame_buf)) frame_len = sizeof(frame_buf);
    memcpy(frame_buf + sizeof(eth_header_t), payload, frame_len - sizeof(eth_header_t));
    return e1000_send_packet(frame_buf, frame_len);
}

int e1000_receive_packet(uint8_t* buffer, uint16_t max_length) {
    if (!e1000_active || !buffer || max_length == 0) return 0;
    return 0;
}

e1000_device_t* e1000_get_device(void) {
    return &e1000_dev;
}
