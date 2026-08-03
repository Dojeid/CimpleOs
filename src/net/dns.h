#ifndef DNS_H
#define DNS_H

#include <stdint.h>

#define DNS_MAX_HOSTNAME 128
#define DNS_PORT 53

typedef struct {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed)) dns_header_t;

int dns_resolve(const char* hostname, uint32_t* out_ip);
void dns_set_server(uint32_t server_ip);
uint32_t dns_get_server(void);
void dns_receive(const uint8_t* pkt, uint16_t len);

#endif // DNS_H
