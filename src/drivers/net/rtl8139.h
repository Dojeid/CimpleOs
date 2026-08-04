#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

#define RTL8139_RX_BUF_SIZE 8192
#define RTL8139_RX_PAD_SIZE 16

typedef struct {
    uint16_t io_base;
    uint8_t  mac[6];
    uint8_t* rx_buffer;
    uint32_t rx_offset;
    int      link_up;
    int      tx_slot;
} rtl8139_device_t;

int               rtl8139_init(void);
rtl8139_device_t* rtl8139_get_device(void);
int               rtl8139_send_packet(const uint8_t* data, uint16_t len);
int               rtl8139_receive_packet(uint8_t* buf, uint16_t max_len);

#ifdef __cplusplus
}
#endif

#endif // RTL8139_H
