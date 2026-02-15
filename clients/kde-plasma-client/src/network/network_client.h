#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct network_client_s network_client_t;
struct frame_buffer_t;  // Defined in .c file

// Callback types
typedef void (*frame_callback_t)(void *user_data, 
                                 uint8_t *y_data, uint8_t *uv_data,
                                 int width, int height, uint64_t timestamp);
typedef void (*error_callback_t)(void *user_data, const char *error_msg);

// Network client structure
struct network_client_s {
    // Socket
    int socket_fd;
    char *host;
    int port;
    struct sockaddr_in server_addr;
    
    // Connection state
    bool connected;
    bool handshake_complete;
    uint64_t peer_id;
    
    // Crypto keys (libsodium)
    uint8_t local_public_key[32];
    uint8_t local_secret_key[32];
    uint8_t remote_public_key[32];
    uint8_t shared_secret[32];
    uint64_t tx_nonce;
    uint64_t rx_nonce;
    
    // Threading
    pthread_t receive_thread;
    pthread_mutex_t mutex;
    bool running;
    
    // Callbacks
    void *user_data;
    frame_callback_t on_frame;
    error_callback_t on_error;
    
    // Frame reassembly buffers
    struct frame_buffer_t *frame_buffers[16];  // MAX_PENDING_FRAMES
    
    // Keepalive timing
    uint64_t last_ping_sent;
    uint64_t last_pong_received;
    
    // Error message buffer
    char last_error[256];
};

// Lifecycle functions
network_client_t* network_client_create(const char *host, int port);
int network_client_init_crypto(network_client_t *client);
void network_client_destroy(network_client_t *client);

// Connection management
int network_client_connect(network_client_t *client);
void network_client_disconnect(network_client_t *client);
bool network_client_is_connected(const network_client_t *client);

// Callback registration
void network_client_set_frame_callback(network_client_t *client, 
                                      frame_callback_t callback,
                                      void *user_data);
void network_client_set_error_callback(network_client_t *client,
                                      error_callback_t callback,
                                      void *user_data);

// Error handling
const char* network_client_get_error(const network_client_t *client);

// Handshake functions
int network_client_start_handshake(network_client_t *client);
int network_client_process_handshake_response(network_client_t *client,
                                              const uint8_t *data,
                                              size_t len);

#ifdef __cplusplus
}
#endif

#endif // NETWORK_CLIENT_H
