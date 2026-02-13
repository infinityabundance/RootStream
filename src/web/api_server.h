/**
 * PHASE 19: Web Dashboard - REST API Server
 * 
 * Provides HTTP/HTTPS REST API for monitoring and control
 */

#ifndef API_SERVER_H
#define API_SERVER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * HTTP request structure
 */
typedef struct {
    const char *path;
    const char *method;         // GET, POST, PUT, DELETE
    const char *query_string;
    const char *body_data;
    size_t body_size;
    const char *client_ip;
    const char *authorization;
} http_request_t;

/**
 * Request handler callback
 */
typedef int (*request_handler_t)(const http_request_t *req,
                                 char **response_body,
                                 size_t *response_size,
                                 char **content_type);

/**
 * API server configuration
 */
typedef struct {
    uint16_t port;              // Default 8080
    bool enable_https;
    const char *cert_file;
    const char *key_file;
    uint32_t max_connections;
    uint32_t timeout_seconds;
} api_server_config_t;

/**
 * API server handle
 */
typedef struct api_server api_server_t;

/**
 * Initialize API server
 */
api_server_t *api_server_init(const api_server_config_t *config);

/**
 * Register route handler
 */
int api_server_register_route(api_server_t *server,
                              const char *path,
                              const char *method,
                              request_handler_t handler);

/**
 * Start API server
 */
int api_server_start(api_server_t *server);

/**
 * Stop API server
 */
int api_server_stop(api_server_t *server);

/**
 * Cleanup API server
 */
void api_server_cleanup(api_server_t *server);

/**
 * Helper: Send JSON response
 */
int api_send_json_response(char **response_body,
                           size_t *response_size,
                           char **content_type,
                           const char *json_string);

/**
 * Helper: Send error response
 */
int api_send_error_response(char **response_body,
                            size_t *response_size,
                            char **content_type,
                            int status_code,
                            const char *error_message);

#ifdef __cplusplus
}
#endif

#endif // API_SERVER_H
