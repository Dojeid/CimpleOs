#include "net/ip.h"
#include "net/tcp.h"
#include "lib/string.h"
#include "lib/printf.h"

// e1000 declarations
extern int e1000_send_packet(const uint8_t* data, uint16_t len);
typedef struct { uint8_t mac[6]; int link_up; } e1000_device_t;
extern e1000_device_t* e1000_get_device(void);

static uint32_t ip_self = MAKE_IP(10,0,2,15);
static uint32_t ip_gateway = MAKE_IP(10,0,2,2);
static uint32_t ip_netmask = MAKE_IP(255,255,255,0);

static arp_entry_t arp_cache[ARP_CACHE_SIZE];
static int arp_count = 0;

void ip_init(void) {
    arp_count = 0;
}

uint16_t ip_checksum(const void* data, uint16_t len) {
    const uint16_t* p = (const uint16_t*)data;
    uint32_t sum = 0;
    while (len > 1) {
        sum += *p++;
        len -= 2;
    }
    if (len == 1) {
        uint16_t temp = 0;
        *(uint8_t*)&temp = *(const uint8_t*)p;
        sum += temp;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16_t)~sum;
}

static uint16_t htons(uint16_t v) {
    return (v >> 8) | (v << 8);
}

static uint32_t htonl(uint32_t v) {
    return ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) |
           ((v & 0xFF0000) >> 8) | ((v & 0xFF000000) >> 24);
}

void arp_resolve(uint32_t ip, uint8_t* mac_out) {
    for (int i = 0; i < arp_count; i++) {
        if (arp_cache[i].ip == ip) {
            for(int j=0; j<6; j++) mac_out[j] = arp_cache[i].mac[j];
            return;
        }
    }
    // Dummy: return broadcast if not found and we pretend to send an ARP request
    for(int j=0; j<6; j++) mac_out[j] = 0xFF;
}

int ip_send(uint32_t dst_ip, uint8_t proto, const uint8_t* payload, uint16_t len) {
    uint8_t mac[6];
    arp_resolve(dst_ip, mac);
    
    uint16_t total_len = sizeof(ip_header_t) + len;
    uint8_t buffer[2048];
    if (total_len + 14 > sizeof(buffer)) return -1;
    
    // Ethernet header
    for(int i=0; i<6; i++) buffer[i] = mac[i];
    e1000_device_t* nic = e1000_get_device();
    if(nic) {
        for(int i=0; i<6; i++) buffer[i+6] = nic->mac[i];
    } else {
        for(int i=0; i<6; i++) buffer[i+6] = 0;
    }
    buffer[12] = 0x08; // IPv4 ether type
    buffer[13] = 0x00;
    
    ip_header_t* ip_hdr = (ip_header_t*)(buffer + 14);
    ip_hdr->version_ihl = 0x45;
    ip_hdr->tos = 0;
    ip_hdr->total_len = htons(total_len);
    ip_hdr->id = 0;
    ip_hdr->flags_frag = 0;
    ip_hdr->ttl = 64;
    ip_hdr->protocol = proto;
    ip_hdr->src_ip = htonl(ip_self);
    ip_hdr->dst_ip = htonl(dst_ip);
    ip_hdr->checksum = 0;
    ip_hdr->checksum = ip_checksum(ip_hdr, sizeof(ip_header_t));
    
    uint8_t* ptr = buffer + 14 + sizeof(ip_header_t);
    for(uint16_t i=0; i<len; i++) ptr[i] = payload[i];
    
    return e1000_send_packet(buffer, total_len + 14);
}

void ip_receive(const uint8_t* pkt, uint16_t len) {
    if (len < 14 + sizeof(ip_header_t)) return;
    ip_header_t* ip = (ip_header_t*)(pkt + 14);
    uint32_t src_ip = htonl(ip->src_ip);
    uint16_t ip_len = htons(ip->total_len);
    if (ip_len > len - 14) return;
    
    if (ip->protocol == IP_PROTO_TCP) {
        tcp_receive(pkt + 14 + sizeof(ip_header_t), ip_len - sizeof(ip_header_t), src_ip);
    }
    // Ignore ICMP for simplicity for now
}

uint32_t ip_from_str(const char* s) {
    uint32_t res = 0;
    int val = 0;
    while (*s) {
        if (*s == '.') {
            res = (res << 8) | (val & 0xFF);
            val = 0;
        } else if (*s >= '0' && *s <= '9') {
            val = val * 10 + (*s - '0');
        }
        s++;
    }
    res = (res << 8) | (val & 0xFF);
    return res;
}

void ip_to_str(uint32_t ip, char* out) {
    sprintf(out, "%d.%d.%d.%d",
        (ip >> 24) & 0xFF,
        (ip >> 16) & 0xFF,
        (ip >> 8) & 0xFF,
        ip & 0xFF);
}
