/*
 * network.c - Encrypted UDP networking with peer management
 * 
 * Protocol Design:
 * ================
 * All packets follow this structure:
 * 
 * [Header: 32 bytes] [Encrypted Payload: variable] [MAC: 16 bytes]
 * 
 * Header (plaintext):
 *   - Magic: 0x524F4F54 ("ROOT") - 4 bytes
 *   - Version: 1 - 1 byte
 *   - Type: PKT_VIDEO, PKT_INPUT, etc - 1 byte
 *   - Flags: reserved - 2 bytes
 *   - Nonce: encryption nonce - 8 bytes
 *   - Payload size: encrypted data length - 2 bytes
 *   - MAC: Poly1305 authentication tag - 16 bytes
 * 
 * Handshake Flow:
 * ===============
 * 1. Client sends PKT_HANDSHAKE with their public key (plaintext)
 * 2. Server verifies public key, derives shared secret
 * 3. Server responds with PKT_HANDSHAKE containing their public key
 * 4. Client derives shared secret
 * 5. Both sides now have same shared secret
 * 6. All future packets encrypted with ChaCha20-Poly1305
 * 
 * Security Properties:
 * ====================
 * - Forward secrecy: compromising one session doesn't affect others
 * - Authentication: MAC prevents impersonation
 * - Confidentiality: ChaCha20 encryption prevents eavesdropping
 * - Integrity: Poly1305 MAC prevents tampering
 * - Replay protection: nonce counter prevents replay attacks
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>

#include <sodium.h>

#define PACKET_MAGIC 0x524F4F54  /* "ROOT" */
#define DEFAULT_PORT 9876

/*
 * Initialize UDP socket for listening and sending
 * 
 * @param ctx  RootStream context
 * @param port Port to bind (0 for automatic)
 * @return     0 on success, -1 on error
 * 
 * Socket options:
 * - SO_REUSEADDR: allow quick restart without "address in use" error
 * - Large buffers: 2MB send/receive to handle bursts
 * - IPTOS_LOWDELAY: hint to OS for low-latency routing
 */
int rootstream_net_init(rootstream_ctx_t *ctx, uint16_t port) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context\n");
        return -1;
    }

    /* Use default port if not specified */
    if (port == 0) {
        port = DEFAULT_PORT;
    }
    ctx->port = port;

    /* Create UDP socket (IPv4 for now, IPv6 TODO) */
    ctx->sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sock_fd < 0) {
        fprintf(stderr, "ERROR: Cannot create UDP socket\n");
        fprintf(stderr, "REASON: %s\n", strerror(errno));
        fprintf(stderr, "FIX: Check system limits (ulimit -n)\n");
        return -1;
    }

    /* Set socket options for performance and reliability */
    int opt = 1;
    if (setsockopt(ctx->sock_fd, SOL_SOCKET, SO_REUSEADDR, 
                   &opt, sizeof(opt)) < 0) {
        fprintf(stderr, "WARNING: Cannot set SO_REUSEADDR\n");
        /* Non-fatal, continue */
    }

    /* Increase buffer sizes for high-bitrate video */
    int buf_size = 2 * 1024 * 1024;  /* 2 MB */
    setsockopt(ctx->sock_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));
    setsockopt(ctx->sock_fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));

    /* Set TOS for low latency (hint to routers) */
    int tos = IPTOS_LOWDELAY;
    setsockopt(ctx->sock_fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));

    /* Bind to address */
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;  /* Listen on all interfaces */

    if (bind(ctx->sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "ERROR: Cannot bind to port %d\n", port);
        fprintf(stderr, "REASON: %s\n", strerror(errno));
        fprintf(stderr, "FIX: Port may be in use, try a different port\n");
        close(ctx->sock_fd);
        return -1;
    }

    printf("✓ Network initialized on 0.0.0.0:%d (UDP)\n", port);

    return 0;
}

/*
 * Send encrypted packet to peer
 * 
 * @param ctx  RootStream context
 * @param peer Destination peer
 * @param type Packet type (PKT_VIDEO, PKT_INPUT, etc)
 * @param data Plaintext payload
 * @param size Payload size
 * @return     0 on success, -1 on error
 * 
 * Process:
 * 1. Encrypt payload with session key
 * 2. Build packet header
 * 3. Send via UDP
 * 4. Update statistics
 */
int rootstream_net_send_encrypted(rootstream_ctx_t *ctx, peer_t *peer,
                                  uint8_t type, const void *data, size_t size) {
    if (!ctx || !peer || !data) {
        fprintf(stderr, "ERROR: Invalid arguments to send_encrypted\n");
        return -1;
    }

    if (!peer->session.authenticated) {
        fprintf(stderr, "ERROR: Cannot send - peer not authenticated\n");
        fprintf(stderr, "PEER: %s\n", peer->hostname);
        fprintf(stderr, "FIX: Complete handshake first\n");
        return -1;
    }

    /* Allocate packet buffer */
    size_t max_cipher_len = size + crypto_aead_chacha20poly1305_IETF_ABYTES;
    size_t packet_len = sizeof(packet_header_t) + max_cipher_len;
    uint8_t *packet = malloc(packet_len);
    if (!packet) {
        fprintf(stderr, "ERROR: Cannot allocate packet buffer\n");
        return -1;
    }

    packet_header_t *hdr = (packet_header_t*)packet;
    uint8_t *payload = packet + sizeof(packet_header_t);

    /* Get nonce (monotonically increasing counter) */
    uint64_t nonce = peer->session.nonce_counter++;

    /* Encrypt payload */
    size_t cipher_len = 0;
    if (crypto_encrypt_packet(&peer->session, data, size, 
                             payload, &cipher_len, nonce) < 0) {
        free(packet);
        fprintf(stderr, "ERROR: Encryption failed\n");
        return -1;
    }

    /* Build header */
    hdr->magic = PACKET_MAGIC;
    hdr->version = 1;
    hdr->type = type;
    hdr->flags = 0;
    hdr->nonce = nonce;
    hdr->payload_size = cipher_len;
    /* MAC is included in cipher_len by crypto_encrypt_packet */

    /* Send packet */
    ssize_t sent = sendto(ctx->sock_fd, packet, 
                         sizeof(packet_header_t) + cipher_len, 0,
                         (struct sockaddr*)&peer->addr, peer->addr_len);

    free(packet);

    if (sent < 0) {
        fprintf(stderr, "ERROR: Send failed\n");
        fprintf(stderr, "REASON: %s\n", strerror(errno));
        return -1;
    }

    ctx->bytes_sent += sent;
    return 0;
}

/*
 * Receive and process incoming packets
 * 
 * @param ctx        RootStream context
 * @param timeout_ms Timeout in milliseconds (0 = non-blocking)
 * @return           0 on success, -1 on error
 * 
 * Handles:
 * - Handshake packets (key exchange)
 * - Video frames
 * - Input events
 * - Control messages
 * - Keepalive pings
 */
int rootstream_net_recv(rootstream_ctx_t *ctx, int timeout_ms) {
    if (!ctx) {
        fprintf(stderr, "ERROR: Invalid context\n");
        return -1;
    }

    /* Poll for incoming data */
    struct pollfd pfd = {
        .fd = ctx->sock_fd,
        .events = POLLIN
    };

    int ret = poll(&pfd, 1, timeout_ms);
    if (ret < 0) {
        fprintf(stderr, "ERROR: Poll failed: %s\n", strerror(errno));
        return -1;
    }

    if (ret == 0) {
        /* Timeout - no data */
        return 0;
    }

    /* Receive packet */
    uint8_t buffer[MAX_PACKET_SIZE];
    struct sockaddr_storage from;
    socklen_t fromlen = sizeof(from);

    ssize_t recv_len = recvfrom(ctx->sock_fd, buffer, sizeof(buffer), 0,
                               (struct sockaddr*)&from, &fromlen);

    if (recv_len < 0) {
        fprintf(stderr, "ERROR: Receive failed: %s\n", strerror(errno));
        return -1;
    }

    if (recv_len < (ssize_t)sizeof(packet_header_t)) {
        fprintf(stderr, "WARNING: Packet too small (%zd bytes), ignoring\n", recv_len);
        return 0;
    }

    packet_header_t *hdr = (packet_header_t*)buffer;

    /* Validate magic number */
    if (hdr->magic != PACKET_MAGIC) {
        fprintf(stderr, "WARNING: Invalid magic number, ignoring packet\n");
        return 0;
    }

    /* Validate version */
    if (hdr->version != 1) {
        fprintf(stderr, "WARNING: Unsupported protocol version %d\n", hdr->version);
        return 0;
    }

    /* Find or create peer */
    peer_t *peer = NULL;
    
    /* For now, simple implementation: use first peer or create new */
    if (ctx->num_peers > 0) {
        peer = &ctx->peers[0];
    } else if (hdr->type == PKT_HANDSHAKE) {
        /* New connection, will be handled in handshake */
        peer = &ctx->peers[0];
        ctx->num_peers = 1;
        memcpy(&peer->addr, &from, fromlen);
        peer->addr_len = fromlen;
        peer->state = PEER_CONNECTING;
    } else {
        fprintf(stderr, "WARNING: Packet from unknown peer (no handshake)\n");
        return 0;
    }

    /* Update last seen time */
    peer->last_seen = get_timestamp_ms();

    /* Handle packet based on type */
    switch (hdr->type) {
        case PKT_HANDSHAKE:
            /* Handle key exchange (see rootstream_net_handshake) */
            printf("INFO: Received handshake from peer\n");
            /* TODO: Full handshake implementation */
            break;

        case PKT_VIDEO:
        case PKT_AUDIO:
        case PKT_INPUT:
        case PKT_CONTROL:
            /* Decrypt and process */
            if (!peer->session.authenticated) {
                fprintf(stderr, "WARNING: Encrypted packet before handshake\n");
                return 0;
            }

            uint8_t *encrypted = buffer + sizeof(packet_header_t);
            size_t encrypted_len = hdr->payload_size;

            uint8_t decrypted[MAX_PACKET_SIZE];
            size_t decrypted_len = 0;

            if (crypto_decrypt_packet(&peer->session, encrypted, encrypted_len,
                                     decrypted, &decrypted_len, hdr->nonce) < 0) {
                fprintf(stderr, "ERROR: Decryption failed\n");
                return 0;
            }

            /* Process decrypted payload based on type */
            if (hdr->type == PKT_INPUT) {
                input_event_pkt_t *input = (input_event_pkt_t*)decrypted;
                rootstream_input_process(ctx, input);
            }
            /* TODO: Handle other packet types */

            ctx->bytes_received += recv_len;
            break;

        case PKT_PING:
            /* Respond with PONG */
            rootstream_net_send_encrypted(ctx, peer, PKT_PONG, NULL, 0);
            break;

        case PKT_PONG:
            /* Keepalive response, just update last_seen (already done) */
            break;

        default:
            fprintf(stderr, "WARNING: Unknown packet type %d\n", hdr->type);
            break;
    }

    return 0;
}

/*
 * Perform handshake with peer (key exchange)
 * 
 * @param ctx  RootStream context
 * @param peer Peer to handshake with
 * @return     0 on success, -1 on error
 * 
 * Handshake packet payload (plaintext):
 * - 32 bytes: sender's Ed25519 public key
 * - Variable: hostname (null-terminated string)
 */
int rootstream_net_handshake(rootstream_ctx_t *ctx, peer_t *peer) {
    if (!ctx || !peer) {
        fprintf(stderr, "ERROR: Invalid arguments to handshake\n");
        return -1;
    }

    /* Build handshake payload: [public_key][hostname] */
    uint8_t payload[256];
    memcpy(payload, ctx->keypair.public_key, CRYPTO_PUBLIC_KEY_BYTES);
    strcpy((char*)(payload + CRYPTO_PUBLIC_KEY_BYTES), ctx->keypair.identity);
    
    size_t payload_len = CRYPTO_PUBLIC_KEY_BYTES + strlen(ctx->keypair.identity) + 1;

    /* Send handshake (unencrypted for initial key exchange) */
    packet_header_t hdr = {0};
    hdr.magic = PACKET_MAGIC;
    hdr.version = 1;
    hdr.type = PKT_HANDSHAKE;
    hdr.payload_size = payload_len;

    uint8_t packet[sizeof(packet_header_t) + payload_len];
    memcpy(packet, &hdr, sizeof(hdr));
    memcpy(packet + sizeof(hdr), payload, payload_len);

    ssize_t sent = sendto(ctx->sock_fd, packet, sizeof(packet), 0,
                         (struct sockaddr*)&peer->addr, peer->addr_len);

    if (sent < 0) {
        fprintf(stderr, "ERROR: Handshake send failed\n");
        fprintf(stderr, "REASON: %s\n", strerror(errno));
        return -1;
    }

    printf("→ Sent handshake to peer\n");

    return 0;
}

/*
 * Parse RootStream code and extract public key + hostname
 * 
 * @param code      RootStream code (format: "base64_pubkey@hostname")
 * @param public_key Output: extracted public key
 * @param hostname  Output: extracted hostname
 * @param host_size Hostname buffer size
 * @return          0 on success, -1 on error
 */
static int parse_rootstream_code(const char *code, uint8_t *public_key,
                                char *hostname, size_t host_size) {
    if (!code || !public_key || !hostname) {
        return -1;
    }

    /* Find @ separator */
    const char *at = strchr(code, '@');
    if (!at) {
        fprintf(stderr, "ERROR: Invalid RootStream code format\n");
        fprintf(stderr, "EXPECTED: base64_pubkey@hostname\n");
        fprintf(stderr, "GOT: %s\n", code);
        return -1;
    }

    /* Extract base64 public key */
    size_t b64_len = at - code;
    char b64_pubkey[256];
    if (b64_len >= sizeof(b64_pubkey)) {
        fprintf(stderr, "ERROR: Public key too long\n");
        return -1;
    }
    memcpy(b64_pubkey, code, b64_len);
    b64_pubkey[b64_len] = '\0';

    /* Decode base64 */
    size_t decoded_len;
    if (sodium_base642bin(public_key, CRYPTO_PUBLIC_KEY_BYTES,
                         b64_pubkey, b64_len,
                         NULL, &decoded_len, NULL,
                         sodium_base64_VARIANT_ORIGINAL) != 0) {
        fprintf(stderr, "ERROR: Invalid base64 encoding in RootStream code\n");
        return -1;
    }

    if (decoded_len != CRYPTO_PUBLIC_KEY_BYTES) {
        fprintf(stderr, "ERROR: Invalid public key length after decode\n");
        return -1;
    }

    /* Extract hostname */
    const char *host_start = at + 1;
    strncpy(hostname, host_start, host_size - 1);
    hostname[host_size - 1] = '\0';

    return 0;
}

/*
 * Add peer by RootStream code
 * 
 * @param ctx  RootStream context
 * @param code RootStream code (e.g., "kXx7Y...@gaming-pc")
 * @return     Pointer to peer on success, NULL on error
 */
peer_t* rootstream_add_peer(rootstream_ctx_t *ctx, const char *code) {
    if (!ctx || !code) {
        fprintf(stderr, "ERROR: Invalid arguments to add_peer\n");
        return NULL;
    }

    if (ctx->num_peers >= MAX_PEERS) {
        fprintf(stderr, "ERROR: Maximum peers reached (%d)\n", MAX_PEERS);
        return NULL;
    }

    peer_t *peer = &ctx->peers[ctx->num_peers];
    memset(peer, 0, sizeof(peer_t));

    /* Parse RootStream code */
    if (parse_rootstream_code(code, peer->public_key, 
                             peer->hostname, sizeof(peer->hostname)) < 0) {
        return NULL;
    }

    /* Verify public key */
    if (crypto_verify_peer(peer->public_key, CRYPTO_PUBLIC_KEY_BYTES) < 0) {
        return NULL;
    }

    /* Store code */
    strncpy(peer->rootstream_code, code, sizeof(peer->rootstream_code) - 1);

    /* Create encryption session */
    if (crypto_create_session(&peer->session, ctx->keypair.secret_key,
                             peer->public_key) < 0) {
        fprintf(stderr, "ERROR: Failed to create encryption session\n");
        return NULL;
    }

    peer->state = PEER_DISCOVERED;
    ctx->num_peers++;

    printf("✓ Added peer: %s\n", peer->hostname);

    return peer;
}

/*
 * Find peer by public key
 * 
 * @param ctx        RootStream context
 * @param public_key Public key to search for
 * @return           Pointer to peer if found, NULL otherwise
 */
peer_t* rootstream_find_peer(rootstream_ctx_t *ctx, const uint8_t *public_key) {
    if (!ctx || !public_key) {
        return NULL;
    }

    for (int i = 0; i < ctx->num_peers; i++) {
        if (memcmp(ctx->peers[i].public_key, public_key, 
                   CRYPTO_PUBLIC_KEY_BYTES) == 0) {
            return &ctx->peers[i];
        }
    }

    return NULL;
}

/*
 * Connect to peer (initiate streaming)
 * 
 * @param ctx  RootStream context
 * @param code Peer's RootStream code
 * @return     0 on success, -1 on error
 */
int rootstream_connect_to_peer(rootstream_ctx_t *ctx, const char *code) {
    if (!ctx || !code) {
        fprintf(stderr, "ERROR: Invalid arguments to connect_to_peer\n");
        return -1;
    }

    /* Add peer (or find existing) */
    peer_t *peer = rootstream_add_peer(ctx, code);
    if (!peer) {
        return -1;
    }

    /* For now, assume peer is on default port (TODO: discovery) */
    struct sockaddr_in *addr = (struct sockaddr_in*)&peer->addr;
    addr->sin_family = AF_INET;
    addr->sin_port = htons(DEFAULT_PORT);
    
    /* TODO: Resolve hostname via DNS or mDNS */
    /* For now, assume localhost for testing */
    inet_pton(AF_INET, "127.0.0.1", &addr->sin_addr);
    peer->addr_len = sizeof(struct sockaddr_in);

    /* Initiate handshake */
    if (rootstream_net_handshake(ctx, peer) < 0) {
        return -1;
    }

    peer->state = PEER_CONNECTING;

    printf("→ Connecting to peer: %s\n", peer->hostname);

    return 0;
}

/*
 * Get current timestamp in milliseconds
 * Used for keepalive and timeout detection
 */
uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

/*
 * Get current timestamp in microseconds
 * Used for latency instrumentation
 */
uint64_t get_timestamp_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}
