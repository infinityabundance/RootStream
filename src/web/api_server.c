/**
 * PHASE 19: Web Dashboard - REST API Server Implementation
 */

#include "api_server.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

// Stub implementation for now - will be expanded with libmicrohttpd
// when dependencies are properly installed

struct api_server {
    api_server_config_t config;
    void *mhd_daemon;  // struct MHD_Daemon *
    pthread_t thread;
    bool running;
};

/**
 * Initialize API server
 */
api_server_t *api_server_init(const api_server_config_t *config) {
    if (!config) {
        return NULL;
    }

    api_server_t *server = (api_server_t *)calloc(1, sizeof(api_server_t));
    if (!server) {
        return NULL;
    }

    // Copy configuration
    server->config = *config;
    server->running = false;

    return server;
}

/**
 * Register route handler
 */
int api_server_register_route(api_server_t *server,
                              const char *path,
                              const char *method,
                              request_handler_t handler) {
    if (!server || !path || !method || !handler) {
        return -1;
    }

    // TODO: Implement route registration with libmicrohttpd
    // For now, this is a stub
    
    return 0;
}

/**
 * Start API server
 */
int api_server_start(api_server_t *server) {
    if (!server || server->running) {
        return -1;
    }

    // TODO: Start libmicrohttpd daemon
    // For now, just mark as running
    server->running = true;

    printf("API server started on port %u\n", server->config.port);
    
    return 0;
}

/**
 * Stop API server
 */
int api_server_stop(api_server_t *server) {
    if (!server || !server->running) {
        return -1;
    }

    // TODO: Stop libmicrohttpd daemon
    server->running = false;

    printf("API server stopped\n");
    
    return 0;
}

/**
 * Cleanup API server
 */
void api_server_cleanup(api_server_t *server) {
    if (!server) {
        return;
    }

    if (server->running) {
        api_server_stop(server);
    }

    free(server);
}

/**
 * Helper: Send JSON response
 */
int api_send_json_response(char **response_body,
                           size_t *response_size,
                           char **content_type,
                           const char *json_string) {
    if (!response_body || !response_size || !content_type || !json_string) {
        return -1;
    }

    size_t len = strlen(json_string);
    *response_body = (char *)malloc(len + 1);
    if (!*response_body) {
        return -1;
    }

    strcpy(*response_body, json_string);
    *response_size = len;
    *content_type = strdup("application/json");

    return 0;
}

/**
 * Helper: Send error response
 */
int api_send_error_response(char **response_body,
                            size_t *response_size,
                            char **content_type,
                            int status_code,
                            const char *error_message) {
    if (!response_body || !response_size || !content_type || !error_message) {
        return -1;
    }

    // Create JSON error response
    char json[1024];
    snprintf(json, sizeof(json),
             "{\"error\": true, \"status\": %d, \"message\": \"%s\"}",
             status_code, error_message);

    return api_send_json_response(response_body, response_size, content_type, json);
}
