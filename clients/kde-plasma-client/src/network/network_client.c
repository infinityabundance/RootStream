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

// Forward declarations
static uint64_t get_timestamp_microseconds(void);
static int derive_shared_secret(network_client_t *client);

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

// Connect to server (stub for now)
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
    return 0;
}

// Disconnect from server
void network_client_disconnect(network_client_t *client) {
    if (!client) {
        return;
    }
    
    client->running = false;
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
