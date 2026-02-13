package com.rootstream.data.models

/**
 * Connection state for streaming
 */
enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    STREAMING,
    PAUSED,
    ERROR,
    RECONNECTING
}

/**
 * Stream configuration
 */
data class StreamConfig(
    val resolution: Resolution = Resolution.HD_1080P,
    val fpsLimit: Int = 60,
    val bitrate: Int = 10_000_000, // 10 Mbps default
    val codec: VideoCodec = VideoCodec.H264,
    val audioEnabled: Boolean = true,
    val lowLatencyMode: Boolean = true
)

enum class Resolution(val width: Int, val height: Int) {
    HD_720P(1280, 720),
    HD_1080P(1920, 1080),
    QHD_1440P(2560, 1440),
    UHD_4K(3840, 2160);

    override fun toString(): String = "${width}x${height}"
}

enum class VideoCodec {
    H264,
    H265,
    VP9,
    AV1
}

/**
 * Stream statistics
 */
data class StreamStats(
    val fps: Int = 0,
    val latency: Long = 0, // milliseconds
    val bitrate: Long = 0, // bits per second
    val packetsReceived: Long = 0,
    val packetsLost: Long = 0,
    val jitter: Long = 0 // milliseconds
) {
    val packetLossPercentage: Float
        get() = if (packetsReceived > 0) {
            (packetsLost.toFloat() / (packetsReceived + packetsLost)) * 100
        } else 0f
}
