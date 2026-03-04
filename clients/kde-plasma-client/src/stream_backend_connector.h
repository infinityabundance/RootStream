/**
 * @file stream_backend_connector.h
 * @brief Wires the C network backend to the Vulkan renderer for the KDE Plasma client
 *
 * StreamBackendConnector receives decoded NV12 frames from the network_client_t
 * backend and hands them off to the Vulkan pipeline
 * (vulkan_upload_frame → vulkan_render → vulkan_present).
 *
 * Usage:
 * @code
 *   StreamBackendConnector conn(vulkanCtx);
 *   conn.onFrameReceived = [](const frame_t *f){ ... };
 *   conn.connect("192.168.1.1", 7777);
 *   conn.start();
 *   // ...
 *   conn.stop();
 *   conn.disconnect();
 * @endcode
 */

#ifndef STREAM_BACKEND_CONNECTOR_H
#define STREAM_BACKEND_CONNECTOR_H

#include <functional>
#include <string>
#include <cstdint>

/* Pull in the C renderer types */
#include "renderer/renderer.h"
#include "renderer/vulkan_renderer.h"

/* Pull in the C network client */
#include "network/network_client.h"

namespace RootStream {

/**
 * @brief Connection state reported to onConnectionStateChanged.
 */
enum class ConnectionState {
    Disconnected,  /**< No active connection */
    Connecting,    /**< TCP handshake in progress */
    Connected,     /**< Streaming active */
    Error          /**< Unrecoverable error; call disconnect() to reset */
};

/**
 * @brief Bridges the streaming network backend to the Vulkan renderer.
 *
 * Thread safety: connect/disconnect/start/stop must be called from a single
 * owner thread.  Frame callbacks are invoked from the network receive thread.
 */
class StreamBackendConnector {
public:
    /**
     * @brief Construct a connector that targets an existing Vulkan context.
     * @param vulkan_ctx  Initialised Vulkan context (ownership not transferred)
     */
    explicit StreamBackendConnector(vulkan_context_t *vulkan_ctx);

    /**
     * @brief Destructor – calls stop() and disconnect() if still running.
     */
    ~StreamBackendConnector();

    /* Non-copyable, non-movable */
    StreamBackendConnector(const StreamBackendConnector &) = delete;
    StreamBackendConnector &operator=(const StreamBackendConnector &) = delete;

    // -------------------------------------------------------------------------
    // Callbacks (set before calling connect())
    // -------------------------------------------------------------------------

    /** Called (from receive thread) each time a complete frame is rendered. */
    std::function<void(const frame_t *)> onFrameReceived;

    /** Called when the connection state changes. */
    std::function<void(ConnectionState)> onConnectionStateChanged;

    /** Called when a non-fatal error occurs (e.g., a dropped frame). */
    std::function<void(const std::string &)> onError;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /**
     * @brief Resolve host and create the underlying network_client_t.
     * @param host  Server hostname or IP address
     * @param port  Server port number
     * @return true on success, false if the client could not be created
     */
    bool connect(const std::string &host, int port);

    /**
     * @brief Tear down the network connection and destroy the network_client_t.
     */
    void disconnect();

    /**
     * @brief Start the receive thread and begin streaming.
     * @return true if the connection was established successfully
     */
    bool start();

    /**
     * @brief Stop streaming and join the receive thread.
     */
    void stop();

    /**
     * @brief Query whether the underlying network client reports connected.
     * @return true if the client is connected and the handshake is complete
     */
    bool isConnected() const;

private:
    /** Static trampoline forwarded to onFrameData(). */
    static void frameCallbackTrampoline(void *user_data,
                                        uint8_t *y_data,
                                        uint8_t *uv_data,
                                        int width,
                                        int height,
                                        uint64_t timestamp);

    /** Static trampoline forwarded to onErrorData(). */
    static void errorCallbackTrampoline(void *user_data, const char *error_msg);

    /** Process an incoming decoded frame: upload → render → present. */
    void onFrameData(uint8_t *y_data, uint8_t *uv_data,
                     int width, int height, uint64_t timestamp);

    /** Handle an error reported by the network client. */
    void onErrorData(const char *error_msg);

    /** Update and broadcast a state change. */
    void setState(ConnectionState state);

    vulkan_context_t   *m_vulkan_ctx;   /**< Borrowed Vulkan context */
    network_client_t   *m_net_client;   /**< Owned network client (or NULL) */
    ConnectionState     m_state;        /**< Current connection state */
};

} /* namespace RootStream */

#endif /* STREAM_BACKEND_CONNECTOR_H */
