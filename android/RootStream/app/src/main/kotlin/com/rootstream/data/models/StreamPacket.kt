package com.rootstream.data.models

/**
 * Stream packet for network communication
 * Compatible with RootStream protocol
 */
data class StreamPacket(
    val type: PacketType,
    val timestamp: Long,
    val data: ByteArray,
    val sequenceNumber: Int = 0
) {
    override fun equals(other: Any?): Boolean {
        if (this === other) return true
        if (javaClass != other?.javaClass) return false

        other as StreamPacket

        if (type != other.type) return false
        if (timestamp != other.timestamp) return false
        if (!data.contentEquals(other.data)) return false
        if (sequenceNumber != other.sequenceNumber) return false

        return true
    }

    override fun hashCode(): Int {
        var result = type.hashCode()
        result = 31 * result + timestamp.hashCode()
        result = 31 * result + data.contentHashCode()
        result = 31 * result + sequenceNumber
        return result
    }
}

enum class PacketType(val value: Int) {
    VIDEO_FRAME(1),
    AUDIO_FRAME(2),
    INPUT_EVENT(3),
    CONTROL(4),
    HEARTBEAT(5),
    AUTH(6);

    companion object {
        fun fromValue(value: Int): PacketType? {
            return values().find { it.value == value }
        }
    }
}
