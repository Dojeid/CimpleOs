#ifndef IP_H
#define IP_H

#include <stdint.h>

#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_frag;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
} __attribute__((packed)) ip_header_t;

#define ARP_CACHE_SIZE 16

typedef struct {
    uint32_t ip;
    uint8_t mac[6];
} arp_entry_t;

#define MAKE_IP(a,b,c,d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

void ip_init(void);
int ip_send(uint32_t dst_ip, uint8_t proto, const uint8_t* payload, uint16_t len);
void ip_receive(const uint8_t* pkt, uint16_t len);
uint16_t ip_checksum(const void* data, uint16_t len);
uint32_t ip_from_str(const char* s);
void ip_to_str(uint32_t ip, char* out);
void arp_resolve(uint32_t ip, uint8_t* mac_out);

#endif // IP_H
