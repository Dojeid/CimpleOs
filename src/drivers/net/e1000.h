#ifndef E1000_H
#define E1000_H

#include <stdint.h>
#include <stddef.h>

#define E1000_VENDOR_ID 0x8086
#define E1000_DEV_82540EM 0x100E

typedef struct {
    uint8_t mac[6];
    uint16_t io_base;
    uint8_t* mmio_base;
    int link_up;
} e1000_device_t;

typedef struct __attribute__((packed)) {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} eth_header_t;

int e1000_init(void);
int e1000_send_packet(const uint8_t* packet, uint16_t length);
int e1000_send_ethernet_frame(const uint8_t* dest_mac, uint16_t ethertype, const uint8_t* payload, uint16_t payload_len);
int e1000_receive_packet(uint8_t* buffer, uint16_t max_length);
e1000_device_t* e1000_get_device(void);

#endif // E1000_H
