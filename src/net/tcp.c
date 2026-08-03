#include "net/tcp.h"
#include "net/ip.h"
#include "lib/string.h"

static tcp_socket_t sockets[MAX_TCP_SOCKETS];
static int socket_count = 0;
static uint16_t next_local_port = 49152;

static uint16_t htons(uint16_t v) { return (v >> 8) | (v << 8); }
static uint16_t ntohs(uint16_t v) { return htons(v); }
static uint32_t htonl(uint32_t v) {
    return ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) |
           ((v & 0xFF0000) >> 8) | ((v & 0xFF000000) >> 24);
}
static uint32_t ntohl(uint32_t v) { return htonl(v); }

int tcp_connect(uint32_t ip, uint16_t port) {
    if (socket_count >= MAX_TCP_SOCKETS) return -1;
    int id = socket_count++;
    tcp_socket_t* sock = &sockets[id];
    sock->socket_id = id;
    sock->state = TCP_SYN_SENT;
    sock->remote_ip = ip;
    sock->remote_port = port;
    sock->local_port = next_local_port++;
    sock->seq = 0x12345678;
    sock->ack = 0;
    sock->rx_len = 0;
    
    uint8_t pkt[sizeof(tcp_header_t)];
    for(int i=0; i<(int)sizeof(tcp_header_t); i++) pkt[i]=0;
    tcp_header_t* hdr = (tcp_header_t*)pkt;
    hdr->src_port = htons(sock->local_port);
    hdr->dst_port = htons(sock->remote_port);
    hdr->seq = htonl(sock->seq);
    hdr->ack = htonl(sock->ack);
    hdr->data_offset = (sizeof(tcp_header_t) / 4) << 4;
    hdr->flags = TCP_FLAG_SYN;
    hdr->window = htons(8192);
    
    ip_send(ip, IP_PROTO_TCP, pkt, sizeof(tcp_header_t));
    return id;
}

int tcp_send(int sock, const uint8_t* data, uint16_t len) {
    if (sock < 0 || sock >= socket_count) return -1;
    tcp_socket_t* s = &sockets[sock];
    if (s->state != TCP_ESTABLISHED) return -1;
    
    uint16_t total_len = sizeof(tcp_header_t) + len;
    uint8_t pkt[2048];
    if (total_len > sizeof(pkt)) return -1;
    for(int i=0; i<(int)sizeof(tcp_header_t); i++) pkt[i]=0;
    tcp_header_t* hdr = (tcp_header_t*)pkt;
    
    hdr->src_port = htons(s->local_port);
    hdr->dst_port = htons(s->remote_port);
    hdr->seq = htonl(s->seq);
    hdr->ack = htonl(s->ack);
    hdr->data_offset = (sizeof(tcp_header_t) / 4) << 4;
    hdr->flags = TCP_FLAG_PSH | TCP_FLAG_ACK;
    hdr->window = htons(8192);
    
    for(int i=0; i<len; i++) pkt[sizeof(tcp_header_t) + i] = data[i];
    
    ip_send(s->remote_ip, IP_PROTO_TCP, pkt, total_len);
    s->seq += len;
    return len;
}

int tcp_recv(int sock, uint8_t* buf, uint16_t max_len) {
    if (sock < 0 || sock >= socket_count) return -1;
    tcp_socket_t* s = &sockets[sock];
    
    if (s->rx_len > 0) {
        uint32_t copy_len = s->rx_len > max_len ? max_len : s->rx_len;
        for(uint32_t i=0; i<copy_len; i++) buf[i] = s->rx_buf[i];
        
        uint32_t remain = s->rx_len - copy_len;
        for(uint32_t i=0; i<remain; i++) s->rx_buf[i] = s->rx_buf[copy_len + i];
        s->rx_len = remain;
        return copy_len;
    }
    return 0;
}

void tcp_close(int sock) {
    if (sock < 0 || sock >= socket_count) return;
    tcp_socket_t* s = &sockets[sock];
    
    s->state = TCP_FIN_WAIT_1;
    uint8_t pkt[sizeof(tcp_header_t)];
    for(int i=0; i<(int)sizeof(tcp_header_t); i++) pkt[i]=0;
    tcp_header_t* hdr = (tcp_header_t*)pkt;
    
    hdr->src_port = htons(s->local_port);
    hdr->dst_port = htons(s->remote_port);
    hdr->seq = htonl(s->seq);
    hdr->ack = htonl(s->ack);
    hdr->data_offset = (sizeof(tcp_header_t) / 4) << 4;
    hdr->flags = TCP_FLAG_FIN | TCP_FLAG_ACK;
    hdr->window = htons(8192);
    
    ip_send(s->remote_ip, IP_PROTO_TCP, pkt, sizeof(tcp_header_t));
}

void tcp_receive(const uint8_t* pkt, uint16_t len, uint32_t src_ip) {
    if (len < sizeof(tcp_header_t)) return;
    const tcp_header_t* hdr = (const tcp_header_t*)pkt;
    uint16_t dest_port = ntohs(hdr->dst_port);
    
    tcp_socket_t* s = 0;
    for(int i=0; i<socket_count; i++) {
        if (sockets[i].local_port == dest_port && sockets[i].remote_ip == src_ip) {
            s = &sockets[i];
            break;
        }
    }
    if (!s) return;
    
    int header_len = (hdr->data_offset >> 4) * 4;
    int payload_len = len - header_len;
    
    if (s->state == TCP_SYN_SENT) {
        if ((hdr->flags & TCP_FLAG_SYN) && (hdr->flags & TCP_FLAG_ACK)) {
            s->state = TCP_ESTABLISHED;
            s->ack = ntohl(hdr->seq) + 1;
            s->seq++; // ACK the SYN
            // Send ACK back
            uint8_t ack_pkt[sizeof(tcp_header_t)];
            for(int i=0; i<(int)sizeof(tcp_header_t); i++) ack_pkt[i]=0;
            tcp_header_t* a_hdr = (tcp_header_t*)ack_pkt;
            a_hdr->src_port = htons(s->local_port);
            a_hdr->dst_port = htons(s->remote_port);
            a_hdr->seq = htonl(s->seq);
            a_hdr->ack = htonl(s->ack);
            a_hdr->data_offset = (sizeof(tcp_header_t) / 4) << 4;
            a_hdr->flags = TCP_FLAG_ACK;
            a_hdr->window = htons(8192);
            ip_send(s->remote_ip, IP_PROTO_TCP, ack_pkt, sizeof(tcp_header_t));
        }
    } else if (s->state == TCP_ESTABLISHED) {
        if (payload_len > 0) {
            if (s->rx_len + payload_len <= TCP_RX_BUF) {
                const uint8_t* payload = pkt + header_len;
                for(int i=0; i<payload_len; i++) {
                    s->rx_buf[s->rx_len++] = payload[i];
                }
                s->ack = ntohl(hdr->seq) + payload_len;
                // ACK it
                uint8_t ack_pkt[sizeof(tcp_header_t)];
                for(int i=0; i<(int)sizeof(tcp_header_t); i++) ack_pkt[i]=0;
                tcp_header_t* a_hdr = (tcp_header_t*)ack_pkt;
                a_hdr->src_port = htons(s->local_port);
                a_hdr->dst_port = htons(s->remote_port);
                a_hdr->seq = htonl(s->seq);
                a_hdr->ack = htonl(s->ack);
                a_hdr->data_offset = (sizeof(tcp_header_t) / 4) << 4;
                a_hdr->flags = TCP_FLAG_ACK;
                a_hdr->window = htons(8192);
                ip_send(s->remote_ip, IP_PROTO_TCP, ack_pkt, sizeof(tcp_header_t));
            }
        }
        if (hdr->flags & TCP_FLAG_FIN) {
            s->state = TCP_CLOSE_WAIT;
            s->ack++;
        }
    }
}

int tcp_socket_state(int sock) {
    if (sock < 0 || sock >= socket_count) return TCP_CLOSED;
    return sockets[sock].state;
}
