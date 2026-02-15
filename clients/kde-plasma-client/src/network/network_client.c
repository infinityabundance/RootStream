#include "network_client.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef HAVE_LIBSODIUM
#include <sodium.h>
#endif

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
