/*
 * network_tcp.c - TCP fallback transport when UDP blocked
 * 
 * Encrypted TCP tunnel for unreliable networks.
 * Uses same encryption/packet format as UDP for compatibility.
 * Slower but works everywhere TCP available.
 */

#include "../include/rootstream.h"
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef RS_PLATFORM_WINDOWS
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

typedef struct {
    rs_socket_t fd;            /* TCP socket FD */
    struct sockaddr_in addr;
    bool connected;
    uint64_t connect_time;
    uint8_t read_buffer[MAX_PACKET_SIZE];
    size_t read_offset;
} tcp_peer_ctx_t;

/*
 * Try to establish TCP connection to peer
 */
int rootstream_net_tcp_connect(rootstream_ctx_t *ctx, peer_t *peer) {
    if (!ctx || !peer) return -1;

    rs_socket_t fd = rs_socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == RS_INVALID_SOCKET) {
        int err = rs_socket_error();
        fprintf(stderr, "ERROR: Cannot create TCP socket: %s\n", rs_socket_strerror(err));
        return -1;
    }

    /* Set non-blocking */
#ifndef RS_PLATFORM_WINDOWS
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    u_long mode = 1;
    ioctlsocket(fd, FIONBIO, &mode);
#endif

    struct sockaddr_in *addr = (struct sockaddr_in *)&peer->addr;
    
    /* Connect (non-blocking) */
    if (connect(fd, (struct sockaddr *)addr, peer->addr_len) < 0) {
        int err = rs_socket_error();
#ifndef RS_PLATFORM_WINDOWS
        if (err != EINPROGRESS) {
#else
        if (err != WSAEWOULDBLOCK) {
#endif
            fprintf(stderr, "ERROR: TCP connect failed: %s\n", rs_socket_strerror(err));
            rs_socket_close(fd);
            return -1;
        }
    }

    /* Wait for connection with timeout */
#ifndef RS_PLATFORM_WINDOWS
    struct pollfd pfd = { .fd = fd, .events = POLLOUT };
    int ret = poll(&pfd, 1, 5000);  /* 5 second timeout */
#else
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);
    struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
    int ret = select((int)fd + 1, NULL, &writefds, NULL, &tv);
#endif

    if (ret <= 0) {
        fprintf(stderr, "ERROR: TCP connect timeout\n");
        rs_socket_close(fd);
        return -1;
    }

    /* Check for connection errors */
    int err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &len) < 0 || err != 0) {
        fprintf(stderr, "ERROR: TCP connect error: %s\n", rs_socket_strerror(err));
        rs_socket_close(fd);
        return -1;
    }

    /* Connection successful */
    tcp_peer_ctx_t *tcp = calloc(1, sizeof(tcp_peer_ctx_t));
    if (!tcp) {
        rs_socket_close(fd);
        return -1;
    }

    tcp->fd = fd;
    memcpy(&tcp->addr, addr, sizeof(*addr));
    tcp->connected = true;
    tcp->connect_time = get_timestamp_ms();
    tcp->read_offset = 0;

    peer->transport_priv = tcp;
    peer->transport = TRANSPORT_TCP;
    
    printf("✓ TCP connection established to %s\n", peer->hostname);
    return 0;
}

/*
 * Send packet via TCP
 */
int rootstream_net_tcp_send(rootstream_ctx_t *ctx, peer_t *peer,
                           const uint8_t *data, size_t size) {
    if (!ctx || !peer || !data || size == 0) return -1;
    if (!peer->transport_priv) return -1;

    tcp_peer_ctx_t *tcp = (tcp_peer_ctx_t *)peer->transport_priv;
    if (!tcp->connected) return -1;

    size_t sent = 0;
    while (sent < size) {
#ifndef RS_PLATFORM_WINDOWS
        ssize_t ret = send(tcp->fd, data + sent, size - sent, MSG_NOSIGNAL);
#else
        ssize_t ret = send(tcp->fd, (const char *)(data + sent), (int)(size - sent), 0);
#endif
        
        if (ret < 0) {
            int err = rs_socket_error();
#ifndef RS_PLATFORM_WINDOWS
            if (err == EAGAIN || err == EWOULDBLOCK) {
#else
            if (err == WSAEWOULDBLOCK) {
#endif
                /* Socket buffer full, try again later */
                break;
            } else {
                fprintf(stderr, "ERROR: TCP send failed: %s\n", rs_socket_strerror(err));
                tcp->connected = false;
                return -1;
            }
        }
        
        sent += ret;
    }

    ctx->bytes_sent += sent;
    peer->last_sent = get_timestamp_ms();
    return sent == size ? 0 : -1;
}

/*
 * Receive packet via TCP with reassembly
 */
int rootstream_net_tcp_recv(rootstream_ctx_t *ctx, peer_t *peer,
                           uint8_t *buffer, size_t *buffer_len) {
    if (!ctx || !peer || !buffer || !buffer_len) return -1;
    if (!peer->transport_priv) return -1;

    tcp_peer_ctx_t *tcp = (tcp_peer_ctx_t *)peer->transport_priv;
    if (!tcp->connected) return -1;

    /* Try to read more data */
#ifndef RS_PLATFORM_WINDOWS
    ssize_t ret = recv(tcp->fd, tcp->read_buffer + tcp->read_offset,
                      sizeof(tcp->read_buffer) - tcp->read_offset, MSG_DONTWAIT);
#else
    ssize_t ret = recv(tcp->fd, (char *)(tcp->read_buffer + tcp->read_offset),
                      (int)(sizeof(tcp->read_buffer) - tcp->read_offset), 0);
#endif

    if (ret < 0) {
        int err = rs_socket_error();
#ifndef RS_PLATFORM_WINDOWS
        if (err != EAGAIN && err != EWOULDBLOCK) {
#else
        if (err != WSAEWOULDBLOCK) {
#endif
            fprintf(stderr, "ERROR: TCP recv failed: %s\n", rs_socket_strerror(err));
            tcp->connected = false;
            return -1;
        }
        return 0;  /* No data available */
    }

    if (ret == 0) {
        /* Connection closed */
        fprintf(stderr, "WARNING: TCP peer closed connection\n");
        tcp->connected = false;
        return -1;
    }

    tcp->read_offset += ret;

    /* Try to extract a complete packet */
    if (tcp->read_offset < sizeof(packet_header_t)) {
        return 0;  /* Need more data */
    }

    packet_header_t *hdr = (packet_header_t *)tcp->read_buffer;
    size_t packet_size = sizeof(packet_header_t) + hdr->payload_size;

    if (tcp->read_offset < packet_size) {
        return 0;  /* Need more data */
    }

    /* We have a complete packet */
    memcpy(buffer, tcp->read_buffer, packet_size);
    *buffer_len = packet_size;

    /* Shift remaining data */
    memmove(tcp->read_buffer, tcp->read_buffer + packet_size,
            tcp->read_offset - packet_size);
    tcp->read_offset -= packet_size;

    ctx->bytes_received += packet_size;
    peer->last_received = get_timestamp_ms();
    return 1;  /* Packet ready */
}

/*
 * Cleanup TCP connection
 */
void rootstream_net_tcp_cleanup(peer_t *peer) {
    if (!peer || !peer->transport_priv) return;

    tcp_peer_ctx_t *tcp = (tcp_peer_ctx_t *)peer->transport_priv;
    if (tcp->fd != RS_INVALID_SOCKET) {
        rs_socket_close(tcp->fd);
    }
    free(tcp);
    peer->transport_priv = NULL;
}

/*
 * Check TCP connection health
 */
bool rootstream_net_tcp_is_healthy(peer_t *peer) {
    if (!peer || !peer->transport_priv) return false;
    tcp_peer_ctx_t *tcp = (tcp_peer_ctx_t *)peer->transport_priv;
    return tcp->connected;
}
