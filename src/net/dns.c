#include "net/dns.h"
#include "net/ip.h"
#include "lib/string.h"
#include "lib/printf.h"

static uint32_t dns_server_ip = MAKE_IP(8,8,8,8);

typedef struct {
    char hostname[64];
    uint32_t ip;
} dns_cache_entry_t;

static dns_cache_entry_t dns_cache[8];
static int dns_cache_count = 0;

void dns_set_server(uint32_t server_ip) {
    dns_server_ip = server_ip;
}

uint32_t dns_get_server(void) {
    return dns_server_ip;
}

static uint16_t htons(uint16_t v) { return (v >> 8) | (v << 8); }
static uint16_t ntohs(uint16_t v) { return htons(v); }

int dns_resolve(const char* hostname, uint32_t* out_ip) {
    for (int i=0; i<dns_cache_count; i++) {
        int match = 1;
        const char* s = hostname;
        const char* c = dns_cache[i].hostname;
        while (*s && *c) {
            if (*s != *c) { match = 0; break; }
            s++; c++;
        }
        if (match && !*s && !*c) {
            *out_ip = dns_cache[i].ip;
            return 0;
        }
    }
    
    uint8_t pkt[512];
    for (int i=0; i<512; i++) pkt[i] = 0;
    dns_header_t* hdr = (dns_header_t*)pkt;
    hdr->id = htons(0x1234);
    hdr->flags = htons(0x0100);
    hdr->qdcount = htons(1);
    
    uint8_t* qname = pkt + sizeof(dns_header_t);
    int p = 0;
    const char* h = hostname;
    while (*h) {
        const char* dot = h;
        while (*dot && *dot != '.') dot++;
        int len = dot - h;
        qname[p++] = len;
        for(int i=0; i<len; i++) qname[p++] = h[i];
        if (*dot) h = dot + 1;
        else h = dot;
    }
    qname[p++] = 0;
    
    uint16_t* qtype = (uint16_t*)(qname + p);
    *qtype = htons(1); // A record
    p += 2;
    uint16_t* qclass = (uint16_t*)(qname + p);
    *qclass = htons(1); // IN
    p += 2;
    
    uint16_t len = sizeof(dns_header_t) + p;
    
    uint8_t udp_pkt[512 + 8];
    uint16_t src_port = 12345;
    uint16_t dst_port = DNS_PORT;
    uint16_t udp_len = len + 8;
    
    udp_pkt[0] = src_port >> 8; udp_pkt[1] = src_port & 0xFF;
    udp_pkt[2] = dst_port >> 8; udp_pkt[3] = dst_port & 0xFF;
    udp_pkt[4] = udp_len >> 8;  udp_pkt[5] = udp_len & 0xFF;
    udp_pkt[6] = 0; udp_pkt[7] = 0; // checksum
    
    for(int i=0; i<len; i++) udp_pkt[8+i] = pkt[i];
    
    ip_send(dns_server_ip, IP_PROTO_UDP, udp_pkt, udp_len);
    
    *out_ip = 0;
    return -1;
}

void dns_receive(const uint8_t* pkt, uint16_t len) {
    if (len < 8 + sizeof(dns_header_t)) return;
    
    const uint8_t* dns_data = pkt + 8;
    const dns_header_t* hdr = (const dns_header_t*)dns_data;
    
    if (ntohs(hdr->id) != 0x1234) return;
    if (!(ntohs(hdr->flags) & 0x8000)) return; // Not a response
    
    int ancount = ntohs(hdr->ancount);
    if (ancount == 0) return;
    
    int offset = sizeof(dns_header_t);
    while (dns_data[offset] != 0 && offset < len - 8) {
        offset += dns_data[offset] + 1;
    }
    offset += 1 + 4; // null byte + QTYPE + QCLASS
    
    if (offset >= len - 8) return;
    
    if (dns_data[offset] & 0xC0) {
        offset += 2;
    } else {
        while (dns_data[offset] != 0 && offset < len - 8) {
            offset += dns_data[offset] + 1;
        }
        offset++;
    }
    
    uint16_t type = (dns_data[offset] << 8) | dns_data[offset+1];
    offset += 2;
    uint16_t cls = (dns_data[offset] << 8) | dns_data[offset+1];
    offset += 2;
    offset += 4; // TTL
    uint16_t rdlength = (dns_data[offset] << 8) | dns_data[offset+1];
    offset += 2;
    
    if (type == 1 && cls == 1 && rdlength == 4 && offset + 4 <= len - 8) {
        uint32_t ip = (dns_data[offset] << 24) |
                      (dns_data[offset+1] << 16) |
                      (dns_data[offset+2] << 8) |
                      dns_data[offset+3];
                      
        if (dns_cache_count < 8) {
            dns_cache[dns_cache_count].ip = ip;
            dns_cache[dns_cache_count].hostname[0] = 'a';
            dns_cache[dns_cache_count].hostname[1] = 0;
            dns_cache_count++;
        }
    }
}
