#ifndef TCP_H
#define TCP_H

#include <stdint.h>

#define TCP_CLOSED 0
#define TCP_SYN_SENT 1
#define TCP_SYN_RECEIVED 2
#define TCP_ESTABLISHED 3
#define TCP_FIN_WAIT_1 4
#define TCP_FIN_WAIT_2 5
#define TCP_TIME_WAIT 6
#define TCP_CLOSE_WAIT 7
#define TCP_LAST_ACK 8

#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_ACK 0x10
#define TCP_FLAG_URG 0x20

#define MAX_TCP_SOCKETS 16
#define TCP_RX_BUF 4096

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t data_offset;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
} __attribute__((packed)) tcp_header_t;

typedef struct {
    int state;
    uint32_t remote_ip;
    uint16_t remote_port;
    uint16_t local_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t rx_buf[TCP_RX_BUF];
    uint32_t rx_len;
    int socket_id;
} tcp_socket_t;

int tcp_connect(uint32_t ip, uint16_t port);
int tcp_send(int sock, const uint8_t* data, uint16_t len);
int tcp_recv(int sock, uint8_t* buf, uint16_t max_len);
void tcp_close(int sock);
void tcp_receive(const uint8_t* pkt, uint16_t len, uint32_t src_ip);
int tcp_socket_state(int sock);

#endif // TCP_H
