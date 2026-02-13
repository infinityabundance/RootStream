/*
 * discovery_broadcast.c - UDP broadcast peer discovery
 * 
 * Falls back to broadcast when mDNS unavailable.
 * Broadcasts a discovery packet on local subnet and waits for responses.
 * Works on any LAN without requiring Avahi.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <poll.h>

#define DISCOVERY_BROADCAST_PORT 5555
#define DISCOVERY_MAGIC "ROOTSTREAM_DISCOVER"

typedef struct {
    uint8_t magic[20];        /* "ROOTSTREAM_DISCOVER" */
    uint32_t version;         /* Protocol version */
    char hostname[256];       /* Sender hostname */
    uint16_t listen_port;     /* Port peer is listening on */
    char rootstream_code[ROOTSTREAM_CODE_MAX_LEN]; /* User's RootStream code */
} discovery_broadcast_packet_t;

/*
 * Get local IP address for broadcast
 */
static int get_local_ip(char *ip_buf, size_t ip_len, char *bcast_buf, size_t bcast_len) {
    struct ifaddrs *ifaddr, *ifa;
    int ret = -1;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return -1;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        /* Skip loopback and non-IPv4 */
        if (ifa->ifa_addr->sa_family != AF_INET) continue;
        if (strcmp(ifa->ifa_name, "lo") == 0) continue;

        struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
        struct sockaddr_in *broadcast = (struct sockaddr_in *)ifa->ifa_broadaddr;

        inet_ntop(AF_INET, &sin->sin_addr, ip_buf, ip_len);
        if (broadcast) {
            inet_ntop(AF_INET, &broadcast->sin_addr, bcast_buf, bcast_len);
        } else {
            snprintf(bcast_buf, bcast_len, "255.255.255.255");
        }

        printf("✓ Using interface %s (%s, broadcast %s)\n", 
               ifa->ifa_name, ip_buf, bcast_buf);
        ret = 0;
        break;
    }

    freeifaddrs(ifaddr);
    return ret;
}

/*
 * Broadcast discovery query
 */
int discovery_broadcast_announce(rootstream_ctx_t *ctx) {
    if (!ctx) return -1;

    char local_ip[INET_ADDRSTRLEN];
    char broadcast_ip[INET_ADDRSTRLEN];

    if (get_local_ip(local_ip, sizeof(local_ip), 
                     broadcast_ip, sizeof(broadcast_ip)) < 0) {
        fprintf(stderr, "ERROR: Cannot determine local IP\n");
        return -1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    /* Enable broadcast */
    int broadcast_flag = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_flag, 
                   sizeof(broadcast_flag)) < 0) {
        perror("setsockopt SO_BROADCAST");
        close(sock);
        return -1;
    }

    /* Create announcement packet */
    discovery_broadcast_packet_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    memcpy(pkt.magic, DISCOVERY_MAGIC, strlen(DISCOVERY_MAGIC));
    pkt.version = PROTOCOL_VERSION;
    gethostname(pkt.hostname, sizeof(pkt.hostname));
    pkt.listen_port = ctx->port;
    strncpy(pkt.rootstream_code, ctx->keypair.rootstream_code, 
            sizeof(pkt.rootstream_code) - 1);

    struct sockaddr_in bcast_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(DISCOVERY_BROADCAST_PORT),
    };
    inet_aton(broadcast_ip, &bcast_addr.sin_addr);

    /* Send broadcast */
    ssize_t ret = sendto(sock, &pkt, sizeof(pkt), 0,
                        (struct sockaddr *)&bcast_addr, sizeof(bcast_addr));
    close(sock);

    if (ret < 0) {
        perror("sendto");
        return -1;
    }

    printf("✓ Broadcast discovery announced (%s:%u)\n", local_ip, ctx->port);
    return 0;
}

/*
 * Listen for broadcast discovery queries and respond
 */
int discovery_broadcast_listen(rootstream_ctx_t *ctx, int timeout_ms) {
    if (!ctx) return -1;

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    /* Bind to broadcast port */
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(DISCOVERY_BROADCAST_PORT),
    };

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sock);
        return -1;
    }

    /* Poll with timeout */
    struct pollfd pfd = { .fd = sock, .events = POLLIN };
    int poll_ret = poll(&pfd, 1, timeout_ms);

    if (poll_ret <= 0) {
        close(sock);
        return poll_ret;  /* No data or timeout */
    }

    /* Receive broadcast packet */
    discovery_broadcast_packet_t pkt;
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    ssize_t recv_len = recvfrom(sock, &pkt, sizeof(pkt), 0,
                               (struct sockaddr *)&from_addr, &from_len);
    close(sock);

    if (recv_len < 0) {
        perror("recvfrom");
        return -1;
    }

    if (recv_len != sizeof(pkt)) {
        fprintf(stderr, "WARNING: Invalid discovery packet size\n");
        return 0;
    }

    /* Validate magic */
    if (memcmp(pkt.magic, DISCOVERY_MAGIC, strlen(DISCOVERY_MAGIC)) != 0) {
        return 0;  /* Not a RootStream packet */
    }

    /* Found a peer! */
    char peer_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &from_addr.sin_addr, peer_ip, sizeof(peer_ip));

    printf("✓ Discovered peer: %s (%s:%u, code: %.16s...)\n",
           pkt.hostname, peer_ip, ntohs(pkt.listen_port), pkt.rootstream_code);

    /* Add to peer list if not already present */
    bool already_exists = false;
    for (int i = 0; i < ctx->num_peers; i++) {
        if (strcmp(ctx->peers[i].hostname, pkt.hostname) == 0) {
            already_exists = true;
            break;
        }
    }

    if (!already_exists && ctx->num_peers < MAX_PEERS) {
        peer_t *peer = &ctx->peers[ctx->num_peers];
        memset(peer, 0, sizeof(peer_t));

        /* Parse address */
        struct sockaddr_in *addr = (struct sockaddr_in*)&peer->addr;
        addr->sin_family = AF_INET;
        addr->sin_port = htons(ntohs(pkt.listen_port));
        memcpy(&addr->sin_addr, &from_addr.sin_addr, sizeof(struct in_addr));
        peer->addr_len = sizeof(struct sockaddr_in);

        /* Store peer info */
        strncpy(peer->hostname, pkt.hostname, sizeof(peer->hostname) - 1);
        peer->hostname[sizeof(peer->hostname) - 1] = '\0';

        strncpy(peer->rootstream_code, pkt.rootstream_code,
                sizeof(peer->rootstream_code) - 1);
        peer->rootstream_code[sizeof(peer->rootstream_code) - 1] = '\0';

        peer->state = PEER_DISCOVERED;
        peer->last_seen = get_timestamp_ms();

        ctx->num_peers++;

        printf("  → Added peer: %s (code: %.16s...)\n",
               peer->hostname, peer->rootstream_code);

        /* Save to history */
        discovery_save_peer_to_history(ctx, pkt.hostname, ntohs(pkt.listen_port),
                                      pkt.rootstream_code);
    }

    return 1;  /* Found peer */
}
