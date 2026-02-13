package com.rootstream.data.models

import java.util.UUID

/**
 * Peer model representing a discovered streaming host
 */
data class Peer(
    val id: String = UUID.randomUUID().toString(),
    val name: String,
    val hostname: String,
    val port: Int,
    val ipAddress: String,
    val capabilities: List<String> = emptyList(),
    val lastSeen: Long = System.currentTimeMillis(),
    val isOnline: Boolean = true
) {
    val displayName: String
        get() = "$name ($ipAddress:$port)"
}
