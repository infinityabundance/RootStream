/**
 * PHASE 19: Web Dashboard - WebSocket Server
 * 
 * Provides WebSocket server for real-time updates
 */

#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include "models.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * WebSocket server configuration
 */
typedef struct {
    uint16_t port;              // Default 8081
    bool enable_wss;            // WSS/TLS
    const char *cert_file;
    const char *key_file;
} websocket_server_config_t;

/**
 * WebSocket server handle
 */
typedef struct websocket_server websocket_server_t;

/**
 * Initialize WebSocket server
 */
websocket_server_t *websocket_server_init(const websocket_server_config_t *config);

/**
 * Start WebSocket server
 */
int websocket_server_start(websocket_server_t *server);

/**
 * Stop WebSocket server
 */
int websocket_server_stop(websocket_server_t *server);

/**
 * Broadcast metrics to all connected clients
 */
int websocket_server_broadcast_metrics(websocket_server_t *server,
                                       const metrics_snapshot_t *metrics);

/**
 * Broadcast event to all connected clients
 */
int websocket_server_broadcast_event(websocket_server_t *server,
                                     const char *event_type,
                                     const char *data);

/**
 * Get number of connected clients
 */
int websocket_server_get_client_count(websocket_server_t *server);

/**
 * Cleanup WebSocket server
 */
void websocket_server_cleanup(websocket_server_t *server);

#ifdef __cplusplus
}
#endif

#endif // WEBSOCKET_SERVER_H
