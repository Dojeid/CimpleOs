#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <stdint.h>
#include <stddef.h>

#define MAX_BT_DEVICES 8
#define BT_NAME_LEN    32

typedef struct {
    char name[BT_NAME_LEN + 1];
    uint8_t bd_addr[6];
    int rssi_dbm;
    int is_paired;
    uint32_t device_class;
} bluetooth_device_t;

typedef struct {
    int is_active;
    int is_discoverable;
    uint8_t host_bd_addr[6];
    int device_count;
    bluetooth_device_t devices[MAX_BT_DEVICES];
} bluetooth_state_t;

void               bluetooth_init(void);
bluetooth_state_t* bluetooth_get_state(void);
int                bluetooth_scan(void);
int                bluetooth_pair(const uint8_t* bd_addr);

#endif // BLUETOOTH_H
