/**
 * @file stream_backend_connector.cpp
 * @brief Implementation of StreamBackendConnector
 *
 * Packs the raw Y+UV planes delivered by the network_client frame callback
 * into a frame_t and drives the Vulkan upload → render → present pipeline.
 */

#include "stream_backend_connector.h"

#include <cstring>
#include <cstdio>

namespace RootStream {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

StreamBackendConnector::StreamBackendConnector(vulkan_context_t *vulkan_ctx)
    : m_vulkan_ctx(vulkan_ctx)
    , m_net_client(nullptr)
    , m_state(ConnectionState::Disconnected)
{}

StreamBackendConnector::~StreamBackendConnector()
{
    stop();
    disconnect();
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool StreamBackendConnector::connect(const std::string &host, int port)
{
    if (m_net_client) disconnect();

    m_net_client = network_client_create(host.c_str(), port);
    if (!m_net_client) {
        if (onError) onError("network_client_create failed");
        return false;
    }

    network_client_set_frame_callback(m_net_client,
                                      &StreamBackendConnector::frameCallbackTrampoline,
                                      this);
    network_client_set_error_callback(m_net_client,
                                      &StreamBackendConnector::errorCallbackTrampoline,
                                      this);
    return true;
}

void StreamBackendConnector::disconnect()
{
    if (!m_net_client) return;

    network_client_disconnect(m_net_client);
    network_client_destroy(m_net_client);
    m_net_client = nullptr;
    setState(ConnectionState::Disconnected);
}

bool StreamBackendConnector::start()
{
    if (!m_net_client) return false;

    setState(ConnectionState::Connecting);

    if (network_client_connect(m_net_client) != 0) {
        setState(ConnectionState::Error);
        if (onError) {
            const char *err = network_client_get_error(m_net_client);
            onError(err ? err : "network_client_connect failed");
        }
        return false;
    }

    setState(ConnectionState::Connected);
    return true;
}

void StreamBackendConnector::stop()
{
    if (!m_net_client) return;
    network_client_disconnect(m_net_client);
    setState(ConnectionState::Disconnected);
}

bool StreamBackendConnector::isConnected() const
{
    return m_net_client && network_client_is_connected(m_net_client);
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void StreamBackendConnector::setState(ConnectionState state)
{
    m_state = state;
    if (onConnectionStateChanged) onConnectionStateChanged(state);
}

void StreamBackendConnector::onFrameData(uint8_t *y_data, uint8_t *uv_data,
                                          int width, int height,
                                          uint64_t timestamp)
{
    if (!m_vulkan_ctx || !y_data || !uv_data) return;

    /* Build a frame_t that points directly at the caller's buffers.
     * The Vulkan upload copies the data, so no ownership transfer occurs. */
    size_t y_size  = static_cast<size_t>(width) * static_cast<size_t>(height);
    size_t uv_size = static_cast<size_t>(width) * static_cast<size_t>(height / 2);

    /* We need a contiguous NV12 buffer: Y plane followed by interleaved UV.
     * network_client_t uses a single receive_thread (pthread_t), so this
     * callback is always invoked from that one thread – thread_local is safe. */
    static thread_local uint8_t *tl_buf      = nullptr;
    static thread_local size_t   tl_buf_size = 0;

    size_t total = y_size + uv_size;
    if (total > tl_buf_size) {
        delete[] tl_buf;
        /* Over-allocate by 2× to amortise reallocations when resolution changes. */
        tl_buf_size = total * 2;
        tl_buf      = new uint8_t[tl_buf_size];
    }
    std::memcpy(tl_buf,          y_data,  y_size);
    std::memcpy(tl_buf + y_size, uv_data, uv_size);

    frame_t frame{};
    frame.data         = tl_buf;
    frame.size         = static_cast<uint32_t>(total);
    frame.width        = static_cast<uint32_t>(width);
    frame.height       = static_cast<uint32_t>(height);
    frame.format       = FRAME_FORMAT_NV12;
    frame.timestamp_us = timestamp;
    frame.is_keyframe  = false;

    /* Drive the Vulkan pipeline */
    if (vulkan_upload_frame(m_vulkan_ctx, &frame) != 0) {
        if (onError) onError("vulkan_upload_frame failed");
        return;
    }
    if (vulkan_render(m_vulkan_ctx) != 0) {
        if (onError) onError("vulkan_render failed");
        return;
    }
    if (vulkan_present(m_vulkan_ctx) != 0) {
        if (onError) onError("vulkan_present failed");
        return;
    }

    if (onFrameReceived) onFrameReceived(&frame);
}

void StreamBackendConnector::onErrorData(const char *error_msg)
{
    setState(ConnectionState::Error);
    if (onError) onError(error_msg ? error_msg : "unknown network error");
}

// ---------------------------------------------------------------------------
// Static trampolines
// ---------------------------------------------------------------------------

void StreamBackendConnector::frameCallbackTrampoline(void *user_data,
                                                      uint8_t *y_data,
                                                      uint8_t *uv_data,
                                                      int width,
                                                      int height,
                                                      uint64_t timestamp)
{
    auto *self = static_cast<StreamBackendConnector *>(user_data);
    if (self) self->onFrameData(y_data, uv_data, width, height, timestamp);
}

void StreamBackendConnector::errorCallbackTrampoline(void *user_data,
                                                      const char *error_msg)
{
    auto *self = static_cast<StreamBackendConnector *>(user_data);
    if (self) self->onErrorData(error_msg);
}

} /* namespace RootStream */
