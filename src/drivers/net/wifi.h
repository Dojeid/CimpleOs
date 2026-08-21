#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>
#include <stddef.h>

#define MAX_WIFI_NETWORKS 16
#define WIFI_SSID_LEN     32

typedef struct {
    char ssid[WIFI_SSID_LEN + 1];
    uint8_t bssid[6];
    int rssi_dbm;
    uint8_t channel;
    uint8_t security_type; // 0=Open, 1=WPA2, 2=WPA3
} wifi_ap_t;

typedef struct {
    int is_connected;
    char active_ssid[WIFI_SSID_LEN + 1];
    uint8_t mac_addr[6];
    int signal_percent;
    int ap_count;
    wifi_ap_t networks[MAX_WIFI_NETWORKS];
} wifi_state_t;

void          wifi_init(void);
wifi_state_t* wifi_get_state(void);
int           wifi_scan(void);
int           wifi_connect(const char* ssid, const char* passphrase);
void          wifi_disconnect(void);

#endif // WIFI_H
