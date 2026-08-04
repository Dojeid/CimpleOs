#include "drivers/net/rtl8139.h"
#include "drivers/bus/pci.h"
#include "lib/io.h"
#include "mm/heap.h"
#include "mm/pmm.h"
#include "lib/string.h"
#include "lib/printf.h"
#include "drivers/video/vga.h"

static rtl8139_device_t g_rtl_dev = {0};

int rtl8139_init(void) {
    struct pci_device dev;
    if (!pci_find_by_id(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, &dev)) {
        vga_print("[RTL8139] Realtek RTL8139 PCI NIC not detected.\n");
        return -1;
    }

    uint16_t io_base = (uint16_t)(dev.bar0 & ~0x3);
    g_rtl_dev.io_base = io_base;

    // Enable Bus Mastering & I/O space in PCI Command Register
    uint32_t pci_cmd = pci_read_config(dev.bus, dev.slot, dev.func, 0x04);
    pci_cmd |= 0x05;
    pci_write_config(dev.bus, dev.slot, dev.func, 0x04, pci_cmd);

    // Power on chip
    outb(io_base + 0x52, 0x00);

    // Software reset
    outb(io_base + 0x37, 0x10);
    while ((inb(io_base + 0x37) & 0x10) != 0) {
        // Wait for reset completion
    }

    // Allocate RX buffer
    g_rtl_dev.rx_buffer = (uint8_t*)kmalloc(RTL8139_RX_BUF_SIZE + RTL8139_RX_PAD_SIZE + 15);
    uintptr_t phys_rx = (uintptr_t)g_rtl_dev.rx_buffer;
    outl(io_base + 0x30, (uint32_t)phys_rx);

    // Configure IMR & RCR
    outw(io_base + 0x3C, 0x0005);
    outl(io_base + 0x44, 0x0F | (1 << 7));

    // Enable Receiver & Transmitter
    outb(io_base + 0x37, 0x0C);

    // Read MAC Address
    for (int i = 0; i < 6; i++) {
        g_rtl_dev.mac[i] = inb(io_base + i);
    }
    g_rtl_dev.link_up = 1;
    g_rtl_dev.tx_slot = 0;

    char log[128];
    snprintf(log, sizeof(log), "[RTL8139] Realtek RTL8139 NIC (MAC: %02X:%02X:%02X:%02X:%02X:%02X) Active.\n",
             g_rtl_dev.mac[0], g_rtl_dev.mac[1], g_rtl_dev.mac[2],
             g_rtl_dev.mac[3], g_rtl_dev.mac[4], g_rtl_dev.mac[5]);
    vga_print(log);
    return 0;
}

rtl8139_device_t* rtl8139_get_device(void) {
    return &g_rtl_dev;
}

int rtl8139_send_packet(const uint8_t* data, uint16_t len) {
    if (!g_rtl_dev.io_base || !data || len == 0 || len > 1792) return -1;
    uint16_t io = g_rtl_dev.io_base;
    int slot = g_rtl_dev.tx_slot;

    outl(io + 0x20 + (slot * 4), (uint32_t)(uintptr_t)data);
    outl(io + 0x10 + (slot * 4), (uint32_t)len);

    g_rtl_dev.tx_slot = (slot + 1) % 4;
    return 0;
}

int rtl8139_receive_packet(uint8_t* buf, uint16_t max_len) {
    if (!g_rtl_dev.io_base || !buf) return -1;
    uint16_t io = g_rtl_dev.io_base;
    if (inb(io + 0x37) & 0x01) return 0;
    return 0;
}
