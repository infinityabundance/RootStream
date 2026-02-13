/*
 * discovery_manual.c - Manual peer entry system
 * 
 * Allows user to manually specify peer address or RootStream code.
 * Always available as ultimate fallback.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>

/*
 * Parse RootStream code (uppercase alphanumeric)
 * Example: "ABCD-1234-EFGH-5678" or full format like "key@hostname"
 */
int discovery_parse_rootstream_code(rootstream_ctx_t *ctx, const char *code, 
                                    char *hostname, uint16_t *port) {
    if (!ctx || !code || !hostname || !port) return -1;

    printf("INFO: Attempting to resolve RootStream code: %.32s...\n", code);
    
    /* Check peer history/favorites */
    for (int i = 0; i < ctx->num_peer_history; i++) {
        if (strcmp(ctx->peer_history_entries[i].rootstream_code, code) == 0) {
            strncpy(hostname, ctx->peer_history_entries[i].hostname, 255);
            hostname[255] = '\0';
            *port = ctx->peer_history_entries[i].port;
            printf("✓ Found code in history: %s:%u\n", hostname, *port);
            return 0;
        }
    }

    fprintf(stderr, "ERROR: RootStream code not found in history\n");
    fprintf(stderr, "INFO: Use --peer-add to add a known peer\n");
    return -1;
}

/*
 * Parse IP:port address
 * Examples: "192.168.1.100:5500" or "example.com:5500"
 */
int discovery_parse_address(const char *address, char *hostname, uint16_t *port) {
    if (!address || !hostname || !port) return -1;

    char buf[256];
    strncpy(buf, address, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    /* Find last colon for port */
    char *colon = strrchr(buf, ':');
    if (!colon) {
        fprintf(stderr, "ERROR: Address must be in format: hostname:port\n");
        return -1;
    }

    *colon = '\0';
    
    /* Copy hostname safely (assume 256 byte destination) */
    strncpy(hostname, buf, 255);
    hostname[255] = '\0';

    char *port_str = colon + 1;
    int port_num = atoi(port_str);
    if (port_num <= 0 || port_num > 65535) {
        fprintf(stderr, "ERROR: Invalid port number: %s\n", port_str);
        return -1;
    }

    *port = (uint16_t)port_num;
    printf("✓ Parsed address: %s:%u\n", hostname, *port);
    return 0;
}

/*
 * Connect to manually specified peer
 */
int discovery_manual_add_peer(rootstream_ctx_t *ctx, const char *address_or_code) {
    if (!ctx || !address_or_code) return -1;

    char hostname[256];
    uint16_t port = 9876;  /* Default port */
    int ret = -1;

    /* Try to parse as IP:port first */
    if (strchr(address_or_code, ':')) {
        ret = discovery_parse_address(address_or_code, hostname, &port);
    } else {
        /* Try as RootStream code */
        ret = discovery_parse_rootstream_code(ctx, address_or_code, hostname, &port);
    }

    if (ret < 0) {
        fprintf(stderr, "ERROR: Cannot parse peer address or code\n");
        return -1;
    }

    /* Check if peer already exists */
    for (int i = 0; i < ctx->num_peers; i++) {
        if (strcmp(ctx->peers[i].hostname, hostname) == 0) {
            printf("INFO: Peer %s already exists\n", hostname);
            return 0;
        }
    }

    /* Add peer */
    if (ctx->num_peers >= MAX_PEERS) {
        fprintf(stderr, "ERROR: Maximum number of peers reached\n");
        return -1;
    }

    printf("INFO: Manually connecting to %s:%u\n", hostname, port);
    
    peer_t *peer = &ctx->peers[ctx->num_peers];
    memset(peer, 0, sizeof(peer_t));

    strncpy(peer->hostname, hostname, sizeof(peer->hostname) - 1);
    peer->hostname[sizeof(peer->hostname) - 1] = '\0';
    peer->addr_len = sizeof(struct sockaddr_in);
    
    struct sockaddr_in *addr = (struct sockaddr_in *)&peer->addr;
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    
    /* Try to resolve hostname */
    if (inet_aton(hostname, &addr->sin_addr) == 0) {
        /* Not an IP, try DNS resolution */
        struct hostent *h = gethostbyname(hostname);
        if (!h) {
            fprintf(stderr, "ERROR: Cannot resolve hostname: %s\n", hostname);
            return -1;
        }
        memcpy(&addr->sin_addr, h->h_addr, h->h_length);
    }

    peer->state = PEER_DISCOVERED;
    peer->last_seen = get_timestamp_ms();
    
    ctx->num_peers++;

    printf("✓ Manually added peer: %s (%s:%u)\n", hostname, hostname, port);
    
    /* Save to history */
    discovery_save_peer_to_history(ctx, hostname, port, address_or_code);
    
    return 0;
}

/*
 * Save peer to history for quick reconnect
 */
int discovery_save_peer_to_history(rootstream_ctx_t *ctx, const char *hostname,
                                  uint16_t port, const char *rootstream_code) {
    if (!ctx || !hostname) return -1;

    /* Check if already in history */
    for (int i = 0; i < ctx->num_peer_history; i++) {
        if (strcmp(ctx->peer_history_entries[i].hostname, hostname) == 0 &&
            ctx->peer_history_entries[i].port == port) {
            return 0;  /* Already saved */
        }
    }

    /* Add to history */
    if (ctx->num_peer_history >= MAX_PEER_HISTORY) {
        /* Shift out oldest */
        memmove(&ctx->peer_history_entries[0], &ctx->peer_history_entries[1],
               (MAX_PEER_HISTORY - 1) * sizeof(ctx->peer_history_entries[0]));
        ctx->num_peer_history--;
    }

    peer_history_entry_t *entry = &ctx->peer_history_entries[ctx->num_peer_history];
    memset(entry, 0, sizeof(peer_history_entry_t));
    
    strncpy(entry->hostname, hostname, sizeof(entry->hostname) - 1);
    entry->hostname[sizeof(entry->hostname) - 1] = '\0';
    
    snprintf(entry->address, sizeof(entry->address), "%s:%u", hostname, port);
    
    entry->port = port;
    
    if (rootstream_code) {
        strncpy(entry->rootstream_code, rootstream_code,
               sizeof(entry->rootstream_code) - 1);
        entry->rootstream_code[sizeof(entry->rootstream_code) - 1] = '\0';
    }

    ctx->num_peer_history++;
    printf("✓ Saved peer to history\n");
    return 0;
}

/*
 * List saved peer history
 */
void discovery_list_peer_history(rootstream_ctx_t *ctx) {
    if (!ctx || ctx->num_peer_history == 0) {
        printf("No saved peers\n");
        return;
    }

    printf("\nSaved Peers:\n");
    for (int i = 0; i < ctx->num_peer_history; i++) {
        printf("  %d. %s (%s)\n", i + 1,
              ctx->peer_history_entries[i].hostname,
              ctx->peer_history_entries[i].address);
        if (strlen(ctx->peer_history_entries[i].rootstream_code) > 0) {
            printf("     Code: %.32s...\n", 
                   ctx->peer_history_entries[i].rootstream_code);
        }
    }
    printf("\n");
}
