#include "network_client.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

#ifdef HAVE_LIBSODIUM
#include <sodium.h>
#endif

// Protocol constants
#define PROTOCOL_MAGIC 0x524F4F54  // "ROOT"
#define PROTOCOL_VERSION 1
#define PKT_HANDSHAKE 0x01
#define PKT_VIDEO 0x02
#define PKT_AUDIO 0x03
#define PKT_PING 0x06
#define PKT_PONG 0x07

// Network constants
#define MAX_PACKET_SIZE 1400  // UDP MTU-safe packet size
#define MAX_PENDING_FRAMES 16  // Maximum number of frames being reassembled

// Handshake packet structure
struct handshake_packet_t {
    uint32_t magic;
    uint8_t version;
    uint8_t type;
    uint16_t flags;
    uint8_t public_key[32];
    uint64_t timestamp_us;
    uint8_t signature[64];
} __attribute__((packed));

// Handshake response structure
struct handshake_response_t {
    uint32_t magic;
    uint8_t version;
    uint8_t type;
    uint16_t flags;
    uint8_t public_key[32];
    uint64_t timestamp_us;
    uint8_t signature[64];
    uint64_t peer_id;
} __attribute__((packed));

// Generic packet header structure
struct packet_header_t {
    uint32_t magic;
    uint8_t version;
    uint8_t type;
    uint16_t flags;
    uint16_t size;  // Payload size
} __attribute__((packed));

// Video chunk header (follows packet header for PKT_VIDEO)
struct video_chunk_header_t {
    uint32_t frame_id;
    uint32_t total_size;
    uint32_t offset;
    uint16_t chunk_size;
    uint16_t flags;
    uint64_t timestamp_us;
} __attribute__((packed));

// Frame buffer for reassembly
struct frame_buffer_t {
    uint32_t frame_id;
    uint32_t total_size;
    uint32_t received_size;
    uint8_t *data;
    uint64_t timestamp_us;
    bool in_use;
};

// Ping/Pong packet structure
struct ping_packet_t {
    uint32_t magic;
    uint8_t version;
    uint8_t type;  // PKT_PING or PKT_PONG
    uint16_t flags;
    uint64_t timestamp_us;
} __attribute__((packed));

// Forward declarations
static uint64_t get_timestamp_microseconds(void);
static int derive_shared_secret(network_client_t *client);
static void* receive_thread_func(void *arg);
static int process_video_chunk(network_client_t *client, const uint8_t *packet, size_t len);

// Create a new network client
network_client_t* network_client_create(const char *host, int port) {
    if (!host || port <= 0) {
        return NULL;
    }
    
    network_client_t *client = (network_client_t*)calloc(1, sizeof(network_client_t));
    if (!client) {
        return NULL;
    }
    
    // Initialize fields
    client->socket_fd = -1;
    client->host = strdup(host);
    client->port = port;
    client->connected = false;
    client->handshake_complete = false;
    client->running = false;
    
    // Initialize frame buffers to NULL
    for (int i = 0; i < MAX_PENDING_FRAMES; i++) {
        client->frame_buffers[i] = NULL;
    }
    
    // Initialize keepalive timing
    client->last_ping_sent = 0;
    client->last_pong_received = 0;
    
    // Initialize mutex
    if (pthread_mutex_init(&client->mutex, NULL) != 0) {
        free(client->host);
        free(client);
        return NULL;
    }
    
    snprintf(client->last_error, sizeof(client->last_error), "No error");
    
    return client;
}

// Initialize cryptography
int network_client_init_crypto(network_client_t *client) {
    if (!client) {
        return -1;
    }
    
#ifdef HAVE_LIBSODIUM
    // Initialize libsodium
    if (sodium_init() < 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Failed to initialize libsodium");
        return -1;
    }
    
    // Generate Ed25519 keypair
    if (crypto_sign_keypair(client->local_public_key, client->local_secret_key) != 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Failed to generate keypair");
        return -1;
    }
    
    // Initialize nonces
    client->tx_nonce = 0;
    client->rx_nonce = 0;
    
    return 0;
#else
    snprintf(client->last_error, sizeof(client->last_error),
            "libsodium not available - encryption disabled");
    return -1;
#endif
}

// Destroy network client
void network_client_destroy(network_client_t *client) {
    if (!client) {
        return;
    }
    
    // Disconnect if connected
    if (client->connected) {
        network_client_disconnect(client);
    }
    
    // Free frame buffers
    for (int i = 0; i < MAX_PENDING_FRAMES; i++) {
        if (client->frame_buffers[i]) {
            if (client->frame_buffers[i]->data) {
                free(client->frame_buffers[i]->data);
            }
            free(client->frame_buffers[i]);
            client->frame_buffers[i] = NULL;
        }
    }
    
    // Cleanup
    pthread_mutex_destroy(&client->mutex);
    
    if (client->host) {
        free(client->host);
    }
    
    // Zero out sensitive data
    memset(client->local_secret_key, 0, sizeof(client->local_secret_key));
    memset(client->shared_secret, 0, sizeof(client->shared_secret));
    
    free(client);
}

// Connect to server
int network_client_connect(network_client_t *client) {
    if (!client) {
        return -1;
    }
    
    // Create UDP socket
    client->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client->socket_fd < 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Failed to create socket: %s", strerror(errno));
        return -1;
    }
    
    // Setup server address
    memset(&client->server_addr, 0, sizeof(client->server_addr));
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(client->port);
    
    if (inet_pton(AF_INET, client->host, &client->server_addr.sin_addr) <= 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Invalid address: %s", client->host);
        close(client->socket_fd);
        client->socket_fd = -1;
        return -1;
    }
    
    client->connected = true;
    client->running = true;
    
    // Start receive thread
    if (pthread_create(&client->receive_thread, NULL, receive_thread_func, client) != 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Failed to create receive thread: %s", strerror(errno));
        client->running = false;
        client->connected = false;
        close(client->socket_fd);
        client->socket_fd = -1;
        return -1;
    }
    
    return 0;
}

// Disconnect from server
void network_client_disconnect(network_client_t *client) {
    if (!client) {
        return;
    }
    
    // Signal thread to stop
    client->running = false;
    
    // Wait for receive thread to finish
    if (client->receive_thread) {
        pthread_join(client->receive_thread, NULL);
        client->receive_thread = 0;
    }
    
    client->connected = false;
    client->handshake_complete = false;
    
    if (client->socket_fd >= 0) {
        close(client->socket_fd);
        client->socket_fd = -1;
    }
}

// Check if connected
bool network_client_is_connected(const network_client_t *client) {
    return client && client->connected;
}

// Set frame callback
void network_client_set_frame_callback(network_client_t *client,
                                      frame_callback_t callback,
                                      void *user_data) {
    if (!client) {
        return;
    }
    
    pthread_mutex_lock(&client->mutex);
    client->on_frame = callback;
    client->user_data = user_data;
    pthread_mutex_unlock(&client->mutex);
}

// Set error callback
void network_client_set_error_callback(network_client_t *client,
                                      error_callback_t callback,
                                      void *user_data) {
    if (!client) {
        return;
    }
    
    pthread_mutex_lock(&client->mutex);
    client->on_error = callback;
    pthread_mutex_unlock(&client->mutex);
}

// Get last error message
const char* network_client_get_error(const network_client_t *client) {
    if (!client) {
        return "Invalid client";
    }
    return client->last_error;
}

// Get current timestamp in microseconds
static uint64_t get_timestamp_microseconds(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000ULL + (uint64_t)tv.tv_usec;
}

// Derive shared secret from Ed25519 keys
static int derive_shared_secret(network_client_t *client) {
    if (!client) {
        return -1;
    }
    
#ifdef HAVE_LIBSODIUM
    // Convert Ed25519 keys to X25519 keys for key exchange
    uint8_t local_x25519_sk[32];
    uint8_t local_x25519_pk[32];
    uint8_t remote_x25519_pk[32];
    
    // Convert local secret key
    if (crypto_sign_ed25519_sk_to_curve25519(local_x25519_sk, client->local_secret_key) != 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Failed to convert local secret key to X25519");
        return -1;
    }
    
    // Convert local public key
    if (crypto_sign_ed25519_pk_to_curve25519(local_x25519_pk, client->local_public_key) != 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Failed to convert local public key to X25519");
        return -1;
    }
    
    // Convert remote public key
    if (crypto_sign_ed25519_pk_to_curve25519(remote_x25519_pk, client->remote_public_key) != 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Failed to convert remote public key to X25519");
        return -1;
    }
    
    // Compute shared secret using X25519
    if (crypto_scalarmult(client->shared_secret, local_x25519_sk, remote_x25519_pk) != 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Failed to compute shared secret");
        return -1;
    }
    
    // Initialize nonces
    client->tx_nonce = 0;
    client->rx_nonce = 0;
    
    // Mark handshake as complete
    client->handshake_complete = true;
    
    // Zero out temporary keys
    sodium_memzero(local_x25519_sk, sizeof(local_x25519_sk));
    
    return 0;
#else
    snprintf(client->last_error, sizeof(client->last_error),
            "libsodium not available");
    return -1;
#endif
}

// Start handshake with server
int network_client_start_handshake(network_client_t *client) {
    if (!client) {
        return -1;
    }
    
    if (!client->connected) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Not connected to server");
        return -1;
    }
    
#ifdef HAVE_LIBSODIUM
    // Create handshake packet
    struct handshake_packet_t packet;
    memset(&packet, 0, sizeof(packet));
    
    packet.magic = htonl(PROTOCOL_MAGIC);
    packet.version = PROTOCOL_VERSION;
    packet.type = PKT_HANDSHAKE;
    packet.flags = 0;
    
    // Copy our public key
    memcpy(packet.public_key, client->local_public_key, 32);
    
    // Get timestamp
    uint64_t timestamp = get_timestamp_microseconds();
    packet.timestamp_us = htobe64(timestamp);
    
    // Sign the timestamp with our Ed25519 secret key
    unsigned long long siglen;
    if (crypto_sign_detached(packet.signature, &siglen,
                            (uint8_t*)&packet.timestamp_us, sizeof(packet.timestamp_us),
                            client->local_secret_key) != 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Failed to sign handshake packet");
        return -1;
    }
    
    // Send handshake packet via UDP
    ssize_t sent = sendto(client->socket_fd, &packet, sizeof(packet), 0,
                         (struct sockaddr*)&client->server_addr,
                         sizeof(client->server_addr));
    
    if (sent < 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Failed to send handshake: %s", strerror(errno));
        return -1;
    }
    
    if (sent != sizeof(packet)) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Partial handshake send: %zd/%zu bytes", sent, sizeof(packet));
        return -1;
    }
    
    return 0;
#else
    snprintf(client->last_error, sizeof(client->last_error),
            "libsodium not available");
    return -1;
#endif
}

// Process handshake response from server
int network_client_process_handshake_response(network_client_t *client,
                                              const uint8_t *data,
                                              size_t len) {
    if (!client || !data) {
        return -1;
    }
    
    if (len < sizeof(struct handshake_response_t)) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Handshake response too small: %zu bytes", len);
        return -1;
    }
    
#ifdef HAVE_LIBSODIUM
    const struct handshake_response_t *response = (const struct handshake_response_t*)data;
    
    // Verify magic number
    if (ntohl(response->magic) != PROTOCOL_MAGIC) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Invalid magic number in handshake response");
        return -1;
    }
    
    // Verify version
    if (response->version != PROTOCOL_VERSION) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Unsupported protocol version: %u", response->version);
        return -1;
    }
    
    // Verify packet type
    if (response->type != PKT_HANDSHAKE) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Invalid packet type in response: %u", response->type);
        return -1;
    }
    
    // Verify server signature
    if (crypto_sign_verify_detached(response->signature,
                                    (uint8_t*)&response->timestamp_us,
                                    sizeof(response->timestamp_us),
                                    response->public_key) != 0) {
        snprintf(client->last_error, sizeof(client->last_error),
                "Invalid server signature");
        return -1;
    }
    
    // Store server public key
    memcpy(client->remote_public_key, response->public_key, 32);
    
    // Store assigned peer ID
    client->peer_id = be64toh(response->peer_id);
    
    // Derive shared secret
    if (derive_shared_secret(client) != 0) {
        return -1;
    }
    
    return 0;
#else
    snprintf(client->last_error, sizeof(client->last_error),
            "libsodium not available");
    return -1;
#endif
}

// Process video chunk and reassemble frames
static int process_video_chunk(network_client_t *client, const uint8_t *packet, size_t len) {
    if (!client || !packet) {
        return -1;
    }
    
    // Need at least packet header + video chunk header
    if (len < sizeof(struct packet_header_t) + sizeof(struct video_chunk_header_t)) {
        return -1;
    }
    
    // Skip packet header, get to video chunk header
    const uint8_t *chunk_data = packet + sizeof(struct packet_header_t);
    const struct video_chunk_header_t *chunk_hdr = (const struct video_chunk_header_t*)chunk_data;
    
    uint32_t frame_id = ntohl(chunk_hdr->frame_id);
    uint32_t total_size = ntohl(chunk_hdr->total_size);
    uint32_t offset = ntohl(chunk_hdr->offset);
    uint16_t chunk_size = ntohs(chunk_hdr->chunk_size);
    uint64_t timestamp_us = be64toh(chunk_hdr->timestamp_us);
    
    // Validate chunk parameters
    if (offset + chunk_size > total_size) {
        fprintf(stderr, "Invalid chunk: offset=%u size=%u total=%u\n",
               offset, chunk_size, total_size);
        return -1;
    }
    
    // Find or allocate frame buffer
    struct frame_buffer_t *fb = NULL;
    for (int i = 0; i < MAX_PENDING_FRAMES; i++) {
        if (client->frame_buffers[i] && client->frame_buffers[i]->frame_id == frame_id) {
            fb = client->frame_buffers[i];
            break;
        }
    }
    
    // Allocate new buffer if not found
    if (!fb) {
        // Find free slot
        for (int i = 0; i < MAX_PENDING_FRAMES; i++) {
            if (!client->frame_buffers[i]) {
                fb = (struct frame_buffer_t*)calloc(1, sizeof(struct frame_buffer_t));
                if (!fb) {
                    return -1;
                }
                fb->frame_id = frame_id;
                fb->total_size = total_size;
                fb->received_size = 0;
                fb->data = (uint8_t*)malloc(total_size);
                fb->timestamp_us = timestamp_us;
                fb->in_use = true;
                
                if (!fb->data) {
                    free(fb);
                    return -1;
                }
                
                client->frame_buffers[i] = fb;
                break;
            }
        }
        
        if (!fb) {
            fprintf(stderr, "No free frame buffers (max %d)\n", MAX_PENDING_FRAMES);
            return -1;
        }
    }
    
    // Copy chunk data to frame buffer
    const uint8_t *chunk_payload = chunk_data + sizeof(struct video_chunk_header_t);
    memcpy(fb->data + offset, chunk_payload, chunk_size);
    fb->received_size += chunk_size;
    
    // Check if frame is complete
    if (fb->received_size >= fb->total_size) {
        // Frame complete! Invoke callback
        if (client->on_frame) {
            // For NV12 format: Y plane followed by interleaved UV
            // Assuming width/height can be derived from total_size
            // This is a simplified version - real implementation needs width/height in protocol
            client->on_frame(client->user_data, 
                           fb->data, NULL,  // Y data, UV data (NULL for now)
                           0, 0,  // width, height (0 for now - need protocol extension)
                           fb->timestamp_us);
        }
        
        // Free the frame buffer
        free(fb->data);
        
        // Find and clear the slot
        for (int i = 0; i < MAX_PENDING_FRAMES; i++) {
            if (client->frame_buffers[i] == fb) {
                free(client->frame_buffers[i]);
                client->frame_buffers[i] = NULL;
                break;
            }
        }
    }
    
    return 0;
}

// Send ping packet to server
static int send_ping(network_client_t *client) {
    if (!client || !client->connected) {
        return -1;
    }
    
    struct ping_packet_t packet;
    memset(&packet, 0, sizeof(packet));
    
    packet.magic = htonl(PROTOCOL_MAGIC);
    packet.version = PROTOCOL_VERSION;
    packet.type = PKT_PING;
    packet.flags = 0;
    packet.timestamp_us = htobe64(get_timestamp_microseconds());
    
    ssize_t sent = sendto(client->socket_fd, &packet, sizeof(packet), 0,
                         (struct sockaddr*)&client->server_addr,
                         sizeof(client->server_addr));
    
    if (sent == sizeof(packet)) {
        client->last_ping_sent = get_timestamp_microseconds();
        return 0;
    }
    
    return -1;
}

// Receive thread function - continuously receives packets from server
static void* receive_thread_func(void *arg) {
    network_client_t *client = (network_client_t*)arg;
    if (!client) {
        return NULL;
    }
    
    uint8_t recv_buffer[MAX_PACKET_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len;
    
    // Set socket timeout for responsive shutdown
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;  // 100ms timeout
    if (setsockopt(client->socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        fprintf(stderr, "Warning: Failed to set socket timeout\n");
    }
    
    while (client->running) {
        from_len = sizeof(from_addr);
        
        // Check if we need to send a ping (every 5 seconds)
        uint64_t now = get_timestamp_microseconds();
        if (client->handshake_complete && 
            (now - client->last_ping_sent) > 5000000) {  // 5 seconds
            send_ping(client);
        }
        
        // Check for timeout (15 seconds since last pong)
        if (client->handshake_complete && client->last_pong_received > 0 &&
            (now - client->last_pong_received) > 15000000) {  // 15 seconds
            fprintf(stderr, "Keepalive timeout - connection may be dead\n");
            // Could trigger reconnection here
        }
        
        // Receive packet
        ssize_t received = recvfrom(client->socket_fd, recv_buffer, sizeof(recv_buffer), 0,
                                   (struct sockaddr*)&from_addr, &from_len);
        
        if (received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout - this is normal, continue
                continue;
            } else if (errno == EINTR) {
                // Interrupted - retry
                continue;
            } else {
                // Real error
                fprintf(stderr, "Receive error: %s\n", strerror(errno));
                break;
            }
        }
        
        if (received == 0) {
            // Connection closed (shouldn't happen with UDP, but handle it)
            continue;
        }
        
        // Must have at least the header
        if (received < sizeof(struct packet_header_t)) {
            fprintf(stderr, "Received packet too small: %zd bytes\n", received);
            continue;
        }
        
        // Parse packet header
        struct packet_header_t *header = (struct packet_header_t*)recv_buffer;
        
        // Verify magic number
        if (ntohl(header->magic) != PROTOCOL_MAGIC) {
            fprintf(stderr, "Invalid packet magic: 0x%08x\n", ntohl(header->magic));
            continue;
        }
        
        // Verify version
        if (header->version != PROTOCOL_VERSION) {
            fprintf(stderr, "Unsupported protocol version: %u\n", header->version);
            continue;
        }
        
        // Dispatch based on packet type
        switch (header->type) {
            case PKT_HANDSHAKE:
                // Handshake response
                if (network_client_process_handshake_response(client, recv_buffer, received) == 0) {
                    printf("Handshake completed successfully\n");
                } else {
                    fprintf(stderr, "Handshake processing failed: %s\n", 
                           network_client_get_error(client));
                }
                break;
                
            case PKT_VIDEO:
                // Video frame packet - process chunk and reassemble
                if (process_video_chunk(client, recv_buffer, received) == 0) {
                    // Chunk processed successfully (may or may not complete frame)
                } else {
                    fprintf(stderr, "Failed to process video chunk\n");
                }
                break;
                
            case PKT_AUDIO:
                // Audio packet - future implementation
                fprintf(stderr, "Received audio packet (%zd bytes) - not yet implemented\n", received);
                break;
                
            case PKT_PING:
                // Server sent ping - respond with pong (not typical but handle it)
                fprintf(stderr, "Received ping from server\n");
                break;
                
            case PKT_PONG:
                // Pong response from server - update last pong time
                client->last_pong_received = get_timestamp_microseconds();
                break;
                
            default:
                fprintf(stderr, "Unknown packet type: %u\n", header->type);
                break;
        }
    }
    
    return NULL;
}
