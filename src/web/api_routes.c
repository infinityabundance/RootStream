/**
 * PHASE 19: Web Dashboard - API Route Handlers Implementation
 * PHASE 30: Security - Connect to auth_manager and remove hardcoded tokens
 */

#include "api_routes.h"
#include "models.h"
#include "auth_manager.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// Global auth manager (should be passed through context in production)
static auth_manager_t *g_auth_manager = NULL;

/**
 * Set the auth manager for API routes
 */
void api_routes_set_auth_manager(auth_manager_t *auth) {
    g_auth_manager = auth;
}

/**
 * Simple JSON string value extractor
 * Finds "key":"value" pattern and extracts value
 */
static int extract_json_string(const char *json, const char *key, char *value, size_t value_size) {
    if (!json || !key || !value) {
        return -1;
    }
    
    // Look for "key":
    char search_pattern[256];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);
    const char *key_pos = strstr(json, search_pattern);
    if (!key_pos) {
        return -1;
    }
    
    // Move past the key and colon
    const char *value_start = key_pos + strlen(search_pattern);
    
    // Skip whitespace
    while (*value_start == ' ' || *value_start == '\t' || *value_start == '\n') {
        value_start++;
    }
    
    // Check if value is a string (starts with ")
    if (*value_start != '"') {
        return -1;
    }
    value_start++; // Skip opening quote
    
    // Find closing quote
    const char *value_end = value_start;
    while (*value_end && *value_end != '"') {
        if (*value_end == '\\' && *(value_end + 1)) {
            value_end++; // Skip escaped character
        }
        value_end++;
    }
    
    // Copy value
    size_t len = value_end - value_start;
    if (len >= value_size) {
        len = value_size - 1;
    }
    strncpy(value, value_start, len);
    value[len] = '\0';
    
    return 0;
}

// Host endpoints
int api_route_get_host_info(const http_request_t *req,
                            char **response_body,
                            size_t *response_size,
                            char **content_type) {
    (void)req;  // Unused for now
    
    // Build host info JSON
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    
    char json[2048];
    snprintf(json, sizeof(json),
        "{"
        "\"hostname\": \"%s\","
        "\"platform\": \"Linux\","
        "\"rootstream_version\": \"1.0.0\","
        "\"uptime_seconds\": %lu,"
        "\"is_streaming\": false"
        "}",
        hostname,
        (unsigned long)time(NULL));
    
    return api_send_json_response(response_body, response_size, content_type, json);
}

int api_route_post_host_start(const http_request_t *req,
                              char **response_body,
                              size_t *response_size,
                              char **content_type) {
    (void)req;
    
    char json[] = "{\"success\": true, \"message\": \"Host started\"}";
    return api_send_json_response(response_body, response_size, content_type, json);
}

int api_route_post_host_stop(const http_request_t *req,
                             char **response_body,
                             size_t *response_size,
                             char **content_type) {
    (void)req;
    
    char json[] = "{\"success\": true, \"message\": \"Host stopped\"}";
    return api_send_json_response(response_body, response_size, content_type, json);
}

// Metrics endpoints
int api_route_get_metrics_current(const http_request_t *req,
                                  char **response_body,
                                  size_t *response_size,
                                  char **content_type) {
    (void)req;
    
    // Return mock metrics
    char json[1024];
    snprintf(json, sizeof(json),
        "{"
        "\"fps\": 60,"
        "\"rtt_ms\": 15,"
        "\"jitter_ms\": 2,"
        "\"gpu_util\": 45,"
        "\"gpu_temp\": 65,"
        "\"cpu_util\": 30,"
        "\"bandwidth_mbps\": 25.5,"
        "\"packets_sent\": 150000,"
        "\"packets_lost\": 12,"
        "\"bytes_sent\": 50000000,"
        "\"timestamp_us\": %lu"
        "}",
        (unsigned long)time(NULL) * 1000000UL);
    
    return api_send_json_response(response_body, response_size, content_type, json);
}

int api_route_get_metrics_history(const http_request_t *req,
                                  char **response_body,
                                  size_t *response_size,
                                  char **content_type) {
    (void)req;
    
    // Return mock history (last 10 samples)
    char json[4096] = "{\"fps_history\": [60,59,60,61,60,59,60,60,61,60],"
                      "\"latency_history\": [15,16,14,15,17,15,14,16,15,15],"
                      "\"gpu_util_history\": [45,46,44,45,47,45,44,46,45,45],"
                      "\"cpu_util_history\": [30,31,29,30,32,30,29,31,30,30]}";
    
    return api_send_json_response(response_body, response_size, content_type, json);
}

// Peer endpoints
int api_route_get_peers(const http_request_t *req,
                       char **response_body,
                       size_t *response_size,
                       char **content_type) {
    (void)req;
    
    // Return empty peers list
    char json[] = "{\"peers\": []}";
    return api_send_json_response(response_body, response_size, content_type, json);
}

// Stream endpoints
int api_route_get_streams(const http_request_t *req,
                          char **response_body,
                          size_t *response_size,
                          char **content_type) {
    (void)req;
    
    // Return empty streams list
    char json[] = "{\"streams\": []}";
    return api_send_json_response(response_body, response_size, content_type, json);
}

int api_route_post_stream_record(const http_request_t *req,
                                 char **response_body,
                                 size_t *response_size,
                                 char **content_type) {
    (void)req;
    
    char json[] = "{\"success\": true, \"message\": \"Recording started\"}";
    return api_send_json_response(response_body, response_size, content_type, json);
}

int api_route_post_stream_stop_record(const http_request_t *req,
                                      char **response_body,
                                      size_t *response_size,
                                      char **content_type) {
    (void)req;
    
    char json[] = "{\"success\": true, \"message\": \"Recording stopped\"}";
    return api_send_json_response(response_body, response_size, content_type, json);
}

// Settings endpoints
int api_route_get_settings_video(const http_request_t *req,
                                 char **response_body,
                                 size_t *response_size,
                                 char **content_type) {
    (void)req;
    
    char json[] = "{"
                  "\"width\": 1920,"
                  "\"height\": 1080,"
                  "\"fps\": 60,"
                  "\"bitrate_kbps\": 20000,"
                  "\"encoder\": \"vaapi\","
                  "\"codec\": \"h264\""
                  "}";
    return api_send_json_response(response_body, response_size, content_type, json);
}

int api_route_put_settings_video(const http_request_t *req,
                                 char **response_body,
                                 size_t *response_size,
                                 char **content_type) {
    (void)req;
    
    char json[] = "{\"success\": true, \"message\": \"Video settings updated\"}";
    return api_send_json_response(response_body, response_size, content_type, json);
}

int api_route_get_settings_audio(const http_request_t *req,
                                 char **response_body,
                                 size_t *response_size,
                                 char **content_type) {
    (void)req;
    
    char json[] = "{"
                  "\"output_device\": \"default\","
                  "\"input_device\": \"default\","
                  "\"sample_rate\": 48000,"
                  "\"channels\": 2,"
                  "\"bitrate_kbps\": 128"
                  "}";
    return api_send_json_response(response_body, response_size, content_type, json);
}

int api_route_put_settings_audio(const http_request_t *req,
                                 char **response_body,
                                 size_t *response_size,
                                 char **content_type) {
    (void)req;
    
    char json[] = "{\"success\": true, \"message\": \"Audio settings updated\"}";
    return api_send_json_response(response_body, response_size, content_type, json);
}

int api_route_get_settings_network(const http_request_t *req,
                                   char **response_body,
                                   size_t *response_size,
                                   char **content_type) {
    (void)req;
    
    char json[] = "{"
                  "\"port\": 9090,"
                  "\"target_bitrate_mbps\": 25,"
                  "\"buffer_size_ms\": 100,"
                  "\"enable_tcp_fallback\": true,"
                  "\"enable_encryption\": true"
                  "}";
    return api_send_json_response(response_body, response_size, content_type, json);
}

int api_route_put_settings_network(const http_request_t *req,
                                   char **response_body,
                                   size_t *response_size,
                                   char **content_type) {
    (void)req;
    
    char json[] = "{\"success\": true, \"message\": \"Network settings updated\"}";
    return api_send_json_response(response_body, response_size, content_type, json);
}

// Authentication endpoints
int api_route_post_auth_login(const http_request_t *req,
                              char **response_body,
                              size_t *response_size,
                              char **content_type) {
    if (!g_auth_manager) {
        char error_json[] = "{\"success\": false, \"error\": \"Authentication system not initialized\"}";
        return api_send_json_response(response_body, response_size, content_type, error_json);
    }
    
    if (!req->body_data || req->body_size == 0) {
        char error_json[] = "{\"success\": false, \"error\": \"Missing request body\"}";
        return api_send_json_response(response_body, response_size, content_type, error_json);
    }
    
    // Parse username and password from JSON body
    char username[256] = {0};
    char password[256] = {0};
    
    if (extract_json_string(req->body_data, "username", username, sizeof(username)) != 0 ||
        extract_json_string(req->body_data, "password", password, sizeof(password)) != 0) {
        char error_json[] = "{\"success\": false, \"error\": \"Invalid JSON format or missing credentials\"}";
        return api_send_json_response(response_body, response_size, content_type, error_json);
    }
    
    // Validate input
    if (strlen(username) == 0 || strlen(password) == 0) {
        char error_json[] = "{\"success\": false, \"error\": \"Username and password required\"}";
        return api_send_json_response(response_body, response_size, content_type, error_json);
    }
    
    // Authenticate with auth_manager
    char token[512] = {0};
    if (auth_manager_authenticate(g_auth_manager, username, password, token, sizeof(token)) != 0) {
        char error_json[] = "{\"success\": false, \"error\": \"Invalid credentials\"}";
        return api_send_json_response(response_body, response_size, content_type, error_json);
    }
    
    // Get user role
    char verify_username[256];
    user_role_t role;
    if (auth_manager_verify_token(g_auth_manager, token, verify_username, sizeof(verify_username), &role) != 0) {
        char error_json[] = "{\"success\": false, \"error\": \"Token generation failed\"}";
        return api_send_json_response(response_body, response_size, content_type, error_json);
    }
    
    // Build response with actual token
    char json[1024];
    const char *role_str = (role == ROLE_ADMIN) ? "ADMIN" : 
                          (role == ROLE_OPERATOR) ? "OPERATOR" : "VIEWER";
    snprintf(json, sizeof(json),
        "{"
        "\"success\": true,"
        "\"token\": \"%s\","
        "\"role\": \"%s\","
        "\"username\": \"%s\""
        "}",
        token, role_str, username);
    
    return api_send_json_response(response_body, response_size, content_type, json);
}

int api_route_post_auth_logout(const http_request_t *req,
                               char **response_body,
                               size_t *response_size,
                               char **content_type) {
    if (!g_auth_manager) {
        char error_json[] = "{\"success\": false, \"error\": \"Authentication system not initialized\"}";
        return api_send_json_response(response_body, response_size, content_type, error_json);
    }
    
    // Extract token from Authorization header
    if (req->authorization && strlen(req->authorization) > 7) {
        // Skip "Bearer " prefix if present
        const char *token = req->authorization;
        if (strncmp(token, "Bearer ", 7) == 0) {
            token += 7;
        }
        
        auth_manager_invalidate_session(g_auth_manager, token);
    }
    
    char json[] = "{\"success\": true, \"message\": \"Logged out\"}";
    return api_send_json_response(response_body, response_size, content_type, json);
}

int api_route_get_auth_verify(const http_request_t *req,
                              char **response_body,
                              size_t *response_size,
                              char **content_type) {
    if (!g_auth_manager) {
        char error_json[] = "{\"valid\": false, \"error\": \"Authentication system not initialized\"}";
        return api_send_json_response(response_body, response_size, content_type, error_json);
    }
    
    // Extract token from Authorization header
    if (!req->authorization || strlen(req->authorization) == 0) {
        char error_json[] = "{\"valid\": false, \"error\": \"No authorization token provided\"}";
        return api_send_json_response(response_body, response_size, content_type, error_json);
    }
    
    const char *token = req->authorization;
    // Skip "Bearer " prefix if present
    if (strncmp(token, "Bearer ", 7) == 0) {
        token += 7;
    }
    
    // Verify token
    char username[256];
    user_role_t role;
    if (auth_manager_verify_token(g_auth_manager, token, username, sizeof(username), &role) != 0) {
        char error_json[] = "{\"valid\": false, \"error\": \"Invalid or expired token\"}";
        return api_send_json_response(response_body, response_size, content_type, error_json);
    }
    
    // Build response
    char json[512];
    const char *role_str = (role == ROLE_ADMIN) ? "ADMIN" : 
                          (role == ROLE_OPERATOR) ? "OPERATOR" : "VIEWER";
    snprintf(json, sizeof(json),
        "{"
        "\"valid\": true,"
        "\"username\": \"%s\","
        "\"role\": \"%s\""
        "}",
        username, role_str);
    
    return api_send_json_response(response_body, response_size, content_type, json);
}
                  "\"valid\": true,"
                  "\"username\": \"admin\","
                  "\"role\": \"ADMIN\""
                  "}";
    return api_send_json_response(response_body, response_size, content_type, json);
}
