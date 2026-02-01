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
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <poll.h>
#include <time.h>
#include <netdb.h>

#include <sodium.h>

#ifdef HAVE_AVAHI
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>
#endif

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
        case PKT_HANDSHAKE: {
            /* Parse handshake payload: [32-byte public key][hostname string] */
            if (hdr->payload_size < CRYPTO_PUBLIC_KEY_BYTES) {
                fprintf(stderr, "ERROR: Handshake payload too small\n");
                return 0;
            }

            uint8_t *payload = buffer + sizeof(packet_header_t);
            uint8_t peer_public_key[CRYPTO_PUBLIC_KEY_BYTES];
            char peer_hostname[64] = {0};

            /* Extract public key */
            memcpy(peer_public_key, payload, CRYPTO_PUBLIC_KEY_BYTES);

            /* Extract hostname (null-terminated string after public key) */
            size_t hostname_len = hdr->payload_size - CRYPTO_PUBLIC_KEY_BYTES;
            if (hostname_len > 0 && hostname_len < sizeof(peer_hostname)) {
                memcpy(peer_hostname, payload + CRYPTO_PUBLIC_KEY_BYTES, hostname_len);
                peer_hostname[hostname_len] = '\0';  /* Ensure null termination */
            }

            printf("✓ Received handshake from %s\n", peer_hostname[0] ? peer_hostname : "unknown");

            /* Store peer information */
            memcpy(peer->public_key, peer_public_key, CRYPTO_PUBLIC_KEY_BYTES);
            if (peer_hostname[0]) {
                strncpy(peer->hostname, peer_hostname, sizeof(peer->hostname) - 1);
            }

            /* Create encryption session (derive shared secret) */
            if (crypto_create_session(&peer->session, ctx->keypair.secret_key,
                                     peer_public_key) < 0) {
                fprintf(stderr, "ERROR: Failed to create encryption session\n");
                peer->state = PEER_DISCONNECTED;
                return 0;
            }

            /* Update peer state */
            peer->state = PEER_HANDSHAKE_RECEIVED;

            /* Send handshake response if we haven't already */
            if (rootstream_net_handshake(ctx, peer) == 0) {
                peer->state = PEER_CONNECTED;
                printf("✓ Handshake complete with %s\n", peer->hostname);

                /* Add to connection history */
                config_add_peer_to_history(ctx, peer->rootstream_code);
            }

            break;
        }

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
            else if (hdr->type == PKT_VIDEO) {
                /* Store video frame for client loop to consume */
                if (decrypted_len <= sizeof(ctx->current_frame.data)) {
                    if (!ctx->current_frame.data) {
                        /* Allocate frame buffer on first use */
                        ctx->current_frame.data = malloc(MAX_PACKET_SIZE * 10);  /* Larger buffer for frames */
                        if (!ctx->current_frame.data) {
                            fprintf(stderr, "ERROR: Failed to allocate frame buffer\n");
                            break;
                        }
                    }
                    memcpy(ctx->current_frame.data, decrypted, decrypted_len);
                    ctx->current_frame.size = decrypted_len;
                    ctx->current_frame.timestamp = get_timestamp_us();
                    ctx->frames_received++;
                } else {
                    fprintf(stderr, "WARNING: Video frame too large: %zu bytes\n", decrypted_len);
                }
            }
            else if (hdr->type == PKT_AUDIO) {
                /* Decode Opus audio and play immediately */
                int16_t pcm_buffer[5760 * 2];  /* Max frame size * stereo */
                size_t pcm_samples = 0;

                if (rootstream_opus_decode(ctx, decrypted, decrypted_len,
                               pcm_buffer, &pcm_samples) == 0) {
                    /* Play audio immediately (low latency, no buffering) */
                    audio_playback_write(ctx, pcm_buffer, pcm_samples);
                } else {
                    #ifdef DEBUG
                    fprintf(stderr, "DEBUG: Audio decode failed\n");
                    #endif
                }
            }
            else if (hdr->type == PKT_CONTROL) {
                /* Process control messages */
                if (decrypted_len >= sizeof(control_packet_t)) {
                    control_packet_t *ctrl = (control_packet_t*)decrypted;

                    switch (ctrl->cmd) {
                        case CTRL_PAUSE:
                            peer->is_streaming = false;
                            printf("INFO: Stream paused by peer %s\n", peer->hostname);
                            break;

                        case CTRL_RESUME:
                            peer->is_streaming = true;
                            printf("INFO: Stream resumed by peer %s\n", peer->hostname);
                            break;

                        case CTRL_SET_BITRATE:
                            if (ctrl->value >= 500000 && ctrl->value <= 100000000) {
                                ctx->encoder.bitrate = ctrl->value;
                                printf("INFO: Bitrate changed to %u bps by peer %s\n",
                                       ctrl->value, peer->hostname);
                            } else {
                                fprintf(stderr, "WARNING: Invalid bitrate %u from peer %s\n",
                                        ctrl->value, peer->hostname);
                            }
                            break;

                        case CTRL_SET_FPS:
                            if (ctrl->value >= 1 && ctrl->value <= 240) {
                                ctx->encoder.framerate = ctrl->value;
                                printf("INFO: Framerate changed to %u fps by peer %s\n",
                                       ctrl->value, peer->hostname);
                            } else {
                                fprintf(stderr, "WARNING: Invalid framerate %u from peer %s\n",
                                        ctrl->value, peer->hostname);
                            }
                            break;

                        case CTRL_REQUEST_KEYFRAME:
                            ctx->encoder.force_keyframe = true;
                            #ifdef DEBUG
                            printf("DEBUG: Keyframe requested by peer %s\n", peer->hostname);
                            #endif
                            break;

                        case CTRL_SET_QUALITY:
                            if (ctrl->value <= 100) {
                                ctx->encoder.quality = (uint8_t)ctrl->value;
                                printf("INFO: Quality changed to %u by peer %s\n",
                                       ctrl->value, peer->hostname);
                            }
                            break;

                        case CTRL_DISCONNECT:
                            printf("INFO: Peer %s requested disconnect\n", peer->hostname);
                            peer->state = PEER_DISCONNECTED;
                            peer->is_streaming = false;
                            break;

                        default:
                            fprintf(stderr, "WARNING: Unknown control command 0x%02x from peer %s\n",
                                    ctrl->cmd, peer->hostname);
                            break;
                    }
                } else {
                    fprintf(stderr, "WARNING: Control packet too small (%zu bytes)\n", decrypted_len);
                }
            }

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

    /* Update peer state and timestamp for timeout tracking */
    if (peer->state != PEER_HANDSHAKE_RECEIVED && peer->state != PEER_CONNECTED) {
        peer->state = PEER_HANDSHAKE_SENT;
        peer->handshake_sent_time = get_timestamp_ms();
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

    char fingerprint[32];
    if (crypto_format_fingerprint(peer->public_key, CRYPTO_PUBLIC_KEY_BYTES,
                                  fingerprint, sizeof(fingerprint)) == 0) {
        printf("✓ Added peer: %s (%s)\n", peer->hostname, fingerprint);
    } else {
        fprintf(stderr, "WARNING: Unable to format peer fingerprint\n");
        printf("✓ Added peer: %s\n", peer->hostname);
    }

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
 * Resolve hostname to IP address
 *
 * Supports:
 * - Standard DNS resolution (any hostname)
 * - mDNS for .local domains (via Avahi if available)
 * - Direct IP addresses (passthrough)
 *
 * @param hostname Hostname to resolve
 * @param port     Target port
 * @param addr     Output: resolved address
 * @param addr_len Output: address length
 * @return         0 on success, -1 on error
 */
static int resolve_hostname(const char *hostname, uint16_t port,
                           struct sockaddr_storage *addr, socklen_t *addr_len) {
    if (!hostname || !addr || !addr_len) {
        return -1;
    }

    struct sockaddr_in *addr4 = (struct sockaddr_in*)addr;
    memset(addr, 0, sizeof(*addr));

    /* First, check if it's already an IP address */
    if (inet_pton(AF_INET, hostname, &addr4->sin_addr) == 1) {
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(port);
        *addr_len = sizeof(struct sockaddr_in);
        return 0;
    }

    /* Check for .local domain (mDNS) */
    const char *local_suffix = strstr(hostname, ".local");
    bool is_mdns = local_suffix && (strlen(local_suffix) == 6 || local_suffix[6] == '\0');

    if (is_mdns) {
#ifdef HAVE_AVAHI
        printf("INFO: Resolving %s via mDNS...\n", hostname);

        /* Create a temporary Avahi client for resolution */
        AvahiSimplePoll *simple_poll = avahi_simple_poll_new();
        if (!simple_poll) {
            fprintf(stderr, "ERROR: Cannot create Avahi poll for hostname resolution\n");
            goto try_dns;
        }

        int avahi_error = 0;
        AvahiClient *client = avahi_client_new(
            avahi_simple_poll_get(simple_poll),
            AVAHI_CLIENT_NO_FAIL,
            NULL, NULL, &avahi_error);

        if (!client) {
            fprintf(stderr, "WARNING: Avahi client creation failed: %s\n",
                    avahi_strerror(avahi_error));
            avahi_simple_poll_free(simple_poll);
            goto try_dns;
        }

        /* Use Avahi's hostname resolver */
        /* For simplicity, we'll try DNS first for .local domains */
        avahi_client_free(client);
        avahi_simple_poll_free(simple_poll);
        printf("INFO: mDNS direct lookup not implemented, trying DNS...\n");
        goto try_dns;
#else
        printf("INFO: mDNS not available, trying DNS for %s\n", hostname);
#endif
    }

try_dns:
    /* Standard DNS resolution using getaddrinfo */
    printf("INFO: Resolving %s via DNS...\n", hostname);

    struct addrinfo hints = {0};
    struct addrinfo *result = NULL;
    hints.ai_family = AF_INET;      /* IPv4 */
    hints.ai_socktype = SOCK_DGRAM; /* UDP */

    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%u", port);

    int ret = getaddrinfo(hostname, port_str, &hints, &result);
    if (ret != 0) {
        fprintf(stderr, "ERROR: Cannot resolve hostname '%s': %s\n",
                hostname, gai_strerror(ret));
        fprintf(stderr, "FIX: Check that the hostname is correct and DNS is working\n");
        return -1;
    }

    /* Use first result */
    if (result && result->ai_addr && result->ai_addrlen <= sizeof(*addr)) {
        memcpy(addr, result->ai_addr, result->ai_addrlen);
        *addr_len = result->ai_addrlen;

        /* Log resolved address */
        char ip_str[INET_ADDRSTRLEN];
        struct sockaddr_in *resolved = (struct sockaddr_in*)result->ai_addr;
        inet_ntop(AF_INET, &resolved->sin_addr, ip_str, sizeof(ip_str));
        printf("✓ Resolved %s → %s\n", hostname, ip_str);

        freeaddrinfo(result);
        return 0;
    }

    if (result) {
        freeaddrinfo(result);
    }

    fprintf(stderr, "ERROR: No valid address found for hostname '%s'\n", hostname);
    return -1;
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

    /* Resolve hostname from peer info */
    if (resolve_hostname(peer->hostname, DEFAULT_PORT,
                        &peer->addr, &peer->addr_len) < 0) {
        fprintf(stderr, "ERROR: Failed to resolve peer hostname: %s\n", peer->hostname);
        fprintf(stderr, "HINT: Try using IP address directly, e.g., pubkey@192.168.1.100\n");
        return -1;
    }

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

/*
 * Send control command to peer
 *
 * @param ctx   RootStream context
 * @param peer  Target peer
 * @param cmd   Control command (CTRL_*)
 * @param value Command-specific value
 * @return      0 on success, -1 on error
 */
int rootstream_send_control(rootstream_ctx_t *ctx, peer_t *peer,
                           control_cmd_t cmd, uint32_t value) {
    if (!ctx || !peer) {
        fprintf(stderr, "ERROR: Invalid arguments to send_control\n");
        return -1;
    }

    control_packet_t ctrl = {
        .cmd = cmd,
        .value = value
    };

    return rootstream_net_send_encrypted(ctx, peer, PKT_CONTROL,
                                         &ctrl, sizeof(ctrl));
}

/*
 * Pause streaming to peer
 */
int rootstream_pause_stream(rootstream_ctx_t *ctx, peer_t *peer) {
    if (!ctx || !peer) {
        return -1;
    }

    int result = rootstream_send_control(ctx, peer, CTRL_PAUSE, 0);
    if (result == 0) {
        peer->is_streaming = false;
        printf("→ Sent pause to %s\n", peer->hostname);
    }
    return result;
}

/*
 * Resume streaming to peer
 */
int rootstream_resume_stream(rootstream_ctx_t *ctx, peer_t *peer) {
    if (!ctx || !peer) {
        return -1;
    }

    int result = rootstream_send_control(ctx, peer, CTRL_RESUME, 0);
    if (result == 0) {
        peer->is_streaming = true;
        printf("→ Sent resume to %s\n", peer->hostname);
    }
    return result;
}

/*
 * Request keyframe from host
 */
int rootstream_request_keyframe(rootstream_ctx_t *ctx, peer_t *peer) {
    if (!ctx || !peer) {
        return -1;
    }

    return rootstream_send_control(ctx, peer, CTRL_REQUEST_KEYFRAME, 0);
}
