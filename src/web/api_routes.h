/**
 * PHASE 19: Web Dashboard - API Route Handlers
 * 
 * Implements REST API endpoints for monitoring and control
 */

#ifndef API_ROUTES_H
#define API_ROUTES_H

#include "api_server.h"

#ifdef __cplusplus
extern "C" {
#endif

// Host endpoints
int api_route_get_host_info(const http_request_t *req,
                            char **response_body,
                            size_t *response_size,
                            char **content_type);

int api_route_post_host_start(const http_request_t *req,
                              char **response_body,
                              size_t *response_size,
                              char **content_type);

int api_route_post_host_stop(const http_request_t *req,
                             char **response_body,
                             size_t *response_size,
                             char **content_type);

// Metrics endpoints
int api_route_get_metrics_current(const http_request_t *req,
                                  char **response_body,
                                  size_t *response_size,
                                  char **content_type);

int api_route_get_metrics_history(const http_request_t *req,
                                  char **response_body,
                                  size_t *response_size,
                                  char **content_type);

// Peer endpoints
int api_route_get_peers(const http_request_t *req,
                       char **response_body,
                       size_t *response_size,
                       char **content_type);

// Stream endpoints
int api_route_get_streams(const http_request_t *req,
                          char **response_body,
                          size_t *response_size,
                          char **content_type);

int api_route_post_stream_record(const http_request_t *req,
                                 char **response_body,
                                 size_t *response_size,
                                 char **content_type);

int api_route_post_stream_stop_record(const http_request_t *req,
                                      char **response_body,
                                      size_t *response_size,
                                      char **content_type);

// Settings endpoints
int api_route_get_settings_video(const http_request_t *req,
                                 char **response_body,
                                 size_t *response_size,
                                 char **content_type);

int api_route_put_settings_video(const http_request_t *req,
                                 char **response_body,
                                 size_t *response_size,
                                 char **content_type);

int api_route_get_settings_audio(const http_request_t *req,
                                 char **response_body,
                                 size_t *response_size,
                                 char **content_type);

int api_route_put_settings_audio(const http_request_t *req,
                                 char **response_body,
                                 size_t *response_size,
                                 char **content_type);

int api_route_get_settings_network(const http_request_t *req,
                                   char **response_body,
                                   size_t *response_size,
                                   char **content_type);

int api_route_put_settings_network(const http_request_t *req,
                                   char **response_body,
                                   size_t *response_size,
                                   char **content_type);

// Authentication endpoints
int api_route_post_auth_login(const http_request_t *req,
                              char **response_body,
                              size_t *response_size,
                              char **content_type);

int api_route_post_auth_logout(const http_request_t *req,
                               char **response_body,
                               size_t *response_size,
                               char **content_type);

int api_route_get_auth_verify(const http_request_t *req,
                              char **response_body,
                              size_t *response_size,
                              char **content_type);

#ifdef __cplusplus
}
#endif

#endif // API_ROUTES_H
