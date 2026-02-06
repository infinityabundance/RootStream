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
#include "platform/platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sodium.h>

/* Platform-specific includes for address structures */
#ifndef RS_PLATFORM_WINDOWS
#include <netinet/ip.h>  /* IPTOS_LOWDELAY */
#endif

#ifdef HAVE_AVAHI
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>
#endif

#define PACKET_MAGIC 0x524F4F54  /* "ROOT" */
#define DEFAULT_PORT 9876
#define MAX_VIDEO_FRAME_SIZE (16 * 1024 * 1024)
#define HANDSHAKE_RETRY_MS 1000
#define PEER_TIMEOUT_MS 5000
#define KEEPALIVE_INTERVAL_MS 1000

static peer_t* rootstream_find_peer_by_addr(rootstream_ctx_t *ctx,
                                           const struct sockaddr_storage *addr,
                                           socklen_t addr_len) {
    if (!ctx || !addr) {
        return NULL;
    }

    for (int i = 0; i < ctx->num_peers; i++) {
        peer_t *peer = &ctx->peers[i];
        if (peer->addr_len != addr_len) {
            continue;
        }

        if (addr->ss_family == AF_INET) {
            const struct sockaddr_in *a = (const struct sockaddr_in*)addr;
            const struct sockaddr_in *b = (const struct sockaddr_in*)&peer->addr;
            if (a->sin_port == b->sin_port &&
                memcmp(&a->sin_addr, &b->sin_addr, sizeof(a->sin_addr)) == 0) {
                return peer;
            }
        } else if (addr->ss_family == AF_INET6) {
            const struct sockaddr_in6 *a = (const struct sockaddr_in6*)addr;
            const struct sockaddr_in6 *b = (const struct sockaddr_in6*)&peer->addr;
            if (a->sin6_port == b->sin6_port &&
                memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(a->sin6_addr)) == 0) {
                return peer;
            }
        }
    }

    return NULL;
}

static size_t max_plain_payload_size(void) {
    size_t max_packet = MAX_PACKET_SIZE;
    if (max_packet <= sizeof(packet_header_t) + crypto_aead_chacha20poly1305_IETF_ABYTES) {
        return 0;
    }
    return max_packet - sizeof(packet_header_t) - crypto_aead_chacha20poly1305_IETF_ABYTES;
}

int rootstream_net_send_video(rootstream_ctx_t *ctx, peer_t *peer,
                              const uint8_t *data, size_t size,
                              uint64_t timestamp_us) {
    if (!ctx || !peer || !data || size == 0) {
        fprintf(stderr, "ERROR: Invalid arguments to send_video\n");
        return -1;
    }

    size_t max_plain = max_plain_payload_size();
    if (max_plain <= sizeof(video_chunk_header_t)) {
        fprintf(stderr, "ERROR: Payload size too small for video chunks\n");
        return -1;
    }

    size_t max_chunk = max_plain - sizeof(video_chunk_header_t);
    uint8_t *payload = malloc(sizeof(video_chunk_header_t) + max_chunk);
    if (!payload) {
        fprintf(stderr, "ERROR: Cannot allocate video payload buffer\n");
        return -1;
    }

    uint32_t frame_id = peer->video_tx_frame_id++;
    size_t offset = 0;
    int result = 0;

    while (offset < size) {
        size_t chunk_size = size - offset;
        if (chunk_size > max_chunk) {
            chunk_size = max_chunk;
        }

        video_chunk_header_t header = {
            .frame_id = frame_id,
            .total_size = (uint32_t)size,
            .offset = (uint32_t)offset,
            .chunk_size = (uint16_t)chunk_size,
            .flags = 0,
            .timestamp_us = timestamp_us
        };

        memcpy(payload, &header, sizeof(header));
        memcpy(payload + sizeof(header), data + offset, chunk_size);

        if (rootstream_net_send_encrypted(ctx, peer, PKT_VIDEO,
                                          payload, sizeof(header) + chunk_size) < 0) {
            result = -1;
            break;
        }

        offset += chunk_size;
    }

    free(payload);
    return result;
}

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

    /* Initialize platform networking */
    if (rs_net_init() < 0) {
        fprintf(stderr, "ERROR: Platform network initialization failed\n");
        return -1;
    }

    /* Use default port if not specified */
    if (port == 0) {
        port = DEFAULT_PORT;
    }
    ctx->port = port;

    /* Create UDP socket (IPv4 for now, IPv6 TODO) */
    ctx->sock_fd = rs_socket_create(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sock_fd == RS_INVALID_SOCKET) {
        int err = rs_socket_error();
        fprintf(stderr, "ERROR: Cannot create UDP socket\n");
        fprintf(stderr, "REASON: %s\n", rs_socket_strerror(err));
        fprintf(stderr, "FIX: Check system limits (ulimit -n)\n");
        return -1;
    }

    /* Set socket options for performance and reliability */
    int opt = 1;
    if (rs_socket_setopt(ctx->sock_fd, SOL_SOCKET, SO_REUSEADDR,
                         &opt, sizeof(opt)) < 0) {
        fprintf(stderr, "WARNING: Cannot set SO_REUSEADDR\n");
        /* Non-fatal, continue */
    }

    /* Increase buffer sizes for high-bitrate video */
    int buf_size = 2 * 1024 * 1024;  /* 2 MB */
    rs_socket_setopt(ctx->sock_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));
    rs_socket_setopt(ctx->sock_fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));

#ifndef RS_PLATFORM_WINDOWS
    /* Set TOS for low latency (hint to routers) - Linux/Unix only */
    int tos = IPTOS_LOWDELAY;
    rs_socket_setopt(ctx->sock_fd, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
#endif

    /* Bind to address */
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;  /* Listen on all interfaces */

    if (rs_socket_bind(ctx->sock_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        int err = rs_socket_error();
        fprintf(stderr, "ERROR: Cannot bind to port %d\n", port);
        fprintf(stderr, "REASON: %s\n", rs_socket_strerror(err));
        fprintf(stderr, "FIX: Port may be in use, try a different port\n");
        rs_socket_close(ctx->sock_fd);
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
    if (!ctx || !peer || (!data && size > 0)) {
        fprintf(stderr, "ERROR: Invalid arguments to send_encrypted\n");
        return -1;
    }

    if (!peer->session.authenticated) {
        fprintf(stderr, "ERROR: Cannot send - peer not authenticated\n");
        fprintf(stderr, "PEER: %s\n", peer->hostname);
        fprintf(stderr, "FIX: Complete handshake first\n");
        return -1;
    }

    size_t max_plain = max_plain_payload_size();
    if (size > max_plain) {
        fprintf(stderr, "ERROR: Payload too large for single packet (%zu > %zu)\n",
                size, max_plain);
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
    hdr->version = PROTOCOL_VERSION;
    hdr->type = type;
    hdr->flags = 0;
    hdr->nonce = nonce;
    hdr->payload_size = cipher_len;
    /* MAC is included in cipher_len by crypto_encrypt_packet */

    /* Send packet */
    int sent = rs_socket_sendto(ctx->sock_fd, packet,
                                sizeof(packet_header_t) + cipher_len, 0,
                                (struct sockaddr*)&peer->addr, peer->addr_len);

    free(packet);

    if (sent < 0) {
        int err = rs_socket_error();
        fprintf(stderr, "ERROR: Send failed\n");
        fprintf(stderr, "REASON: %s\n", rs_socket_strerror(err));
        return -1;
    }

    peer->last_sent = get_timestamp_ms();
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
    int ret = rs_socket_poll(ctx->sock_fd, timeout_ms);
    if (ret < 0) {
        int err = rs_socket_error();
        fprintf(stderr, "ERROR: Poll failed: %s\n", rs_socket_strerror(err));
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

    int recv_len = rs_socket_recvfrom(ctx->sock_fd, buffer, sizeof(buffer), 0,
                                      (struct sockaddr*)&from, &fromlen);

    if (recv_len < 0) {
        int err = rs_socket_error();
        fprintf(stderr, "ERROR: Receive failed: %s\n", rs_socket_strerror(err));
        return -1;
    }

    if (recv_len < (int)sizeof(packet_header_t)) {
        fprintf(stderr, "WARNING: Packet too small (%d bytes), ignoring\n", recv_len);
        return 0;
    }

    if (rootstream_net_validate_packet(buffer, (size_t)recv_len) != 0) {
        fprintf(stderr, "WARNING: Invalid packet received (%d bytes)\n", recv_len);
        return 0;
    }

    packet_header_t *hdr = (packet_header_t*)buffer;

    /* Find or create peer */
    peer_t *peer = rootstream_find_peer_by_addr(ctx, &from, fromlen);
    if (!peer) {
        if (hdr->type != PKT_HANDSHAKE) {
            fprintf(stderr, "WARNING: Packet from unknown peer (no handshake)\n");
            return 0;
        }
        if (ctx->num_peers >= MAX_PEERS) {
            fprintf(stderr, "WARNING: Peer limit reached, ignoring handshake\n");
            return 0;
        }

        peer = &ctx->peers[ctx->num_peers++];
        memset(peer, 0, sizeof(peer_t));
        memcpy(&peer->addr, &from, fromlen);
        peer->addr_len = fromlen;
        peer->state = PEER_CONNECTING;
        peer->video_tx_frame_id = 1;
        peer->video_rx_frame_id = 0;
        peer->video_rx_buffer = NULL;
        peer->video_rx_capacity = 0;
        peer->video_rx_expected = 0;
        peer->video_rx_received = 0;
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
            size_t hostname_len = 0;
            if (hdr->payload_size > CRYPTO_PUBLIC_KEY_BYTES) {
                size_t max_len = hdr->payload_size - CRYPTO_PUBLIC_KEY_BYTES;
                size_t i = 0;
                for (; i < max_len; i++) {
                    if (payload[CRYPTO_PUBLIC_KEY_BYTES + i] == '\0') {
                        hostname_len = i;
                        break;
                    }
                }
                if (hostname_len > 0 && hostname_len < sizeof(peer_hostname)) {
                    memcpy(peer_hostname, payload + CRYPTO_PUBLIC_KEY_BYTES, hostname_len);
                    peer_hostname[hostname_len] = '\0';  /* Ensure null termination */
                }
            }

            /* Optional protocol version + flags after hostname */
            size_t extensions_offset = CRYPTO_PUBLIC_KEY_BYTES;
            if (hdr->payload_size > CRYPTO_PUBLIC_KEY_BYTES + hostname_len) {
                extensions_offset = CRYPTO_PUBLIC_KEY_BYTES + hostname_len + 1;
            }
            uint8_t peer_version = PROTOCOL_VERSION;
            uint8_t peer_flags = 0;
            if (hdr->payload_size >= extensions_offset + 2) {
                peer_version = payload[extensions_offset];
                peer_flags = payload[extensions_offset + 1];
            }

            if (peer_version < PROTOCOL_MIN_VERSION || peer_version > PROTOCOL_VERSION) {
                fprintf(stderr, "WARNING: Peer protocol version %u unsupported\n", peer_version);
                peer->state = PEER_DISCONNECTED;
                return 0;
            }

            printf("✓ Received handshake from %s\n", peer_hostname[0] ? peer_hostname : "unknown");

            /* Store peer information */
            memcpy(peer->public_key, peer_public_key, CRYPTO_PUBLIC_KEY_BYTES);
            if (peer_hostname[0]) {
                snprintf(peer->hostname, sizeof(peer->hostname), "%s", peer_hostname);
            }
            peer->protocol_version = peer_version;
            peer->protocol_flags = peer_flags;

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
                if (ctx->is_host) {
                    peer->is_streaming = true;
                }
                printf("✓ Handshake complete with %s\n", peer->hostname);

                /* Add to connection history */
                config_add_peer_to_history(ctx, peer->rootstream_code);

                if (!ctx->is_host) {
                    rootstream_request_keyframe(ctx, peer);
                }
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
                if (decrypted_len < sizeof(video_chunk_header_t)) {
                    fprintf(stderr, "WARNING: Video chunk too small: %zu bytes\n", decrypted_len);
                    break;
                }

                video_chunk_header_t header;
                memcpy(&header, decrypted, sizeof(header));

                if (header.total_size == 0 || header.total_size > MAX_VIDEO_FRAME_SIZE) {
                    fprintf(stderr, "WARNING: Invalid video frame size: %u bytes\n",
                            header.total_size);
                    break;
                }

                if ((size_t)header.offset + header.chunk_size > header.total_size) {
                    fprintf(stderr, "WARNING: Video chunk out of range (offset=%u size=%u total=%u)\n",
                            header.offset, header.chunk_size, header.total_size);
                    break;
                }

                if (decrypted_len != sizeof(video_chunk_header_t) + header.chunk_size) {
                    fprintf(stderr, "WARNING: Video chunk size mismatch\n");
                    break;
                }

                if (peer->video_rx_frame_id != header.frame_id) {
                    peer->video_rx_frame_id = header.frame_id;
                    peer->video_rx_received = 0;
                    peer->video_rx_expected = header.total_size;
                }

                if (peer->video_rx_capacity < peer->video_rx_expected) {
                    uint8_t *new_buf = realloc(peer->video_rx_buffer, peer->video_rx_expected);
                    if (!new_buf) {
                        fprintf(stderr, "ERROR: Failed to allocate video reassembly buffer\n");
                        break;
                    }
                    peer->video_rx_buffer = new_buf;
                    peer->video_rx_capacity = peer->video_rx_expected;
                }

                memcpy(peer->video_rx_buffer + header.offset,
                       decrypted + sizeof(video_chunk_header_t),
                       header.chunk_size);
                peer->video_rx_received += header.chunk_size;

                if (peer->video_rx_received >= peer->video_rx_expected) {
                    ctx->current_frame.data = peer->video_rx_buffer;
                    ctx->current_frame.size = peer->video_rx_expected;
                    ctx->current_frame.capacity = peer->video_rx_capacity;
                    ctx->current_frame.timestamp = header.timestamp_us;
                    ctx->last_video_ts_us = header.timestamp_us;
                    ctx->frames_received++;
                }
            }
            else if (hdr->type == PKT_AUDIO) {
                if (!ctx->settings.audio_enabled) {
                    break;
                }
                /* Decode Opus audio and play immediately */
                if (decrypted_len < sizeof(audio_packet_header_t)) {
                    fprintf(stderr, "WARNING: Audio packet too small: %zu bytes\n", decrypted_len);
                    break;
                }

                audio_packet_header_t header;
                memcpy(&header, decrypted, sizeof(header));

                size_t opus_len = decrypted_len - sizeof(audio_packet_header_t);
                const uint8_t *opus_data = decrypted + sizeof(audio_packet_header_t);
                int16_t pcm_buffer[5760 * 2];  /* Max frame size * stereo */
                size_t pcm_samples = 0;

                if (rootstream_opus_decode(ctx, opus_data, opus_len,
                               pcm_buffer, &pcm_samples) == 0) {
                    bool drop_audio = false;
                    if (ctx->last_video_ts_us > 0) {
                        int64_t delta = (int64_t)header.timestamp_us - (int64_t)ctx->last_video_ts_us;
                        if (delta > 80000 || delta < -200000) {
                            drop_audio = true;
                        }
                    }

                    if (!drop_audio) {
                        audio_playback_write(ctx, pcm_buffer, pcm_samples);
                        ctx->last_audio_ts_us = header.timestamp_us;
                    }
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

void rootstream_net_tick(rootstream_ctx_t *ctx) {
    if (!ctx) {
        return;
    }

    uint64_t now = get_timestamp_ms();
    for (int i = 0; i < ctx->num_peers; i++) {
        peer_t *peer = &ctx->peers[i];

        if (peer->state == PEER_HANDSHAKE_SENT) {
            if (now - peer->handshake_sent_time >= HANDSHAKE_RETRY_MS) {
                rootstream_net_handshake(ctx, peer);
            }
            if (now - peer->handshake_sent_time >= PEER_TIMEOUT_MS) {
                fprintf(stderr, "WARNING: Handshake timeout for peer %s\n",
                        peer->hostname[0] ? peer->hostname : "unknown");
                peer->state = PEER_DISCONNECTED;
            }
        }

        if (peer->state == PEER_CONNECTED) {
            if (peer->last_seen > 0 && now - peer->last_seen >= PEER_TIMEOUT_MS) {
                fprintf(stderr, "WARNING: Peer timeout: %s\n",
                        peer->hostname[0] ? peer->hostname : "unknown");
                peer->state = PEER_DISCONNECTED;
                peer->is_streaming = false;
                if (peer->video_rx_buffer) {
                    free(peer->video_rx_buffer);
                    peer->video_rx_buffer = NULL;
                    peer->video_rx_capacity = 0;
                }
                continue;
            }

            if (now - peer->last_sent >= KEEPALIVE_INTERVAL_MS) {
                rootstream_net_send_encrypted(ctx, peer, PKT_PING, NULL, 0);
                peer->last_ping = now;
            }
        }
    }
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
    if (payload_len + 2 <= sizeof(payload)) {
        payload[payload_len] = PROTOCOL_VERSION;
        payload[payload_len + 1] = PROTOCOL_FLAGS;
        payload_len += 2;
    }

    /* Send handshake (unencrypted for initial key exchange) */
    packet_header_t hdr = {0};
    hdr.magic = PACKET_MAGIC;
    hdr.version = PROTOCOL_VERSION;
    hdr.type = PKT_HANDSHAKE;
    hdr.payload_size = payload_len;

    uint8_t packet[sizeof(packet_header_t) + 256];  /* Fixed size for MSVC compatibility */
    memcpy(packet, &hdr, sizeof(hdr));
    memcpy(packet + sizeof(hdr), payload, payload_len);

    int sent = rs_socket_sendto(ctx->sock_fd, packet, sizeof(hdr) + payload_len, 0,
                                (struct sockaddr*)&peer->addr, peer->addr_len);

    if (sent < 0) {
        int err = rs_socket_error();
        fprintf(stderr, "ERROR: Handshake send failed\n");
        fprintf(stderr, "REASON: %s\n", rs_socket_strerror(err));
        return -1;
    }

    peer->last_sent = get_timestamp_ms();

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

    uint8_t public_key[CRYPTO_PUBLIC_KEY_BYTES];
    char hostname[64] = {0};

    /* Parse RootStream code */
    if (parse_rootstream_code(code, public_key,
                             hostname, sizeof(hostname)) < 0) {
        return NULL;
    }

    peer_t *existing = rootstream_find_peer(ctx, public_key);
    if (existing) {
        strncpy(existing->rootstream_code, code, sizeof(existing->rootstream_code) - 1);
        if (hostname[0]) {
            strncpy(existing->hostname, hostname, sizeof(existing->hostname) - 1);
        }
        if (!existing->session.authenticated) {
            if (crypto_create_session(&existing->session, ctx->keypair.secret_key,
                                     public_key) < 0) {
                fprintf(stderr, "ERROR: Failed to create encryption session\n");
                return NULL;
            }
        }
        return existing;
    }

    if (ctx->num_peers >= MAX_PEERS) {
        fprintf(stderr, "ERROR: Maximum peers reached (%d)\n", MAX_PEERS);
        return NULL;
    }

    peer_t *peer = &ctx->peers[ctx->num_peers];
    memset(peer, 0, sizeof(peer_t));

    memcpy(peer->public_key, public_key, CRYPTO_PUBLIC_KEY_BYTES);
    if (hostname[0]) {
        strncpy(peer->hostname, hostname, sizeof(peer->hostname) - 1);
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
    peer->video_tx_frame_id = 1;
    peer->video_rx_frame_id = 0;
    peer->video_rx_buffer = NULL;
    peer->video_rx_capacity = 0;
    peer->video_rx_expected = 0;
    peer->video_rx_received = 0;
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

void rootstream_remove_peer(rootstream_ctx_t *ctx, peer_t *peer) {
    if (!ctx || !peer) {
        return;
    }

    int index = (int)(peer - ctx->peers);
    if (index < 0 || index >= ctx->num_peers) {
        return;
    }

    if (peer->video_rx_buffer) {
        if (ctx->current_frame.data == peer->video_rx_buffer) {
            ctx->current_frame.data = NULL;
            ctx->current_frame.size = 0;
        }
        free(peer->video_rx_buffer);
        peer->video_rx_buffer = NULL;
    }

    for (int i = index; i < ctx->num_peers - 1; i++) {
        ctx->peers[i] = ctx->peers[i + 1];
    }

    ctx->num_peers--;
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
    return rs_timestamp_ms();
}

/*
 * Get current timestamp in microseconds
 * Used for latency instrumentation
 */
uint64_t get_timestamp_us(void) {
    return rs_timestamp_us();
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
