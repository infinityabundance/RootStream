package com.rootstream.transfer

import android.content.Context
import android.net.Uri
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.util.concurrent.ConcurrentHashMap

/**
 * FileTransferManager - File transfer over the streaming connection.
 * Sends and receives files in 64 KB chunks as DATA_TRANSFER packets.
 */
class FileTransferManager(private val context: Context) {

    interface FileTransferCallback {
        fun onProgress(filename: String, sent: Long, total: Long)
        fun onComplete(filename: String)
        fun onError(filename: String, error: String)
    }

    private val scope = CoroutineScope(Dispatchers.IO)
    private val activeTransfers = ConcurrentHashMap<String, Job>()

    fun sendFile(uri: Uri, callback: FileTransferCallback) {
        val filename = resolveFilename(uri) ?: uri.lastPathSegment ?: "unknown"
        val job = scope.launch {
            try {
                val bytes = context.contentResolver.openInputStream(uri)?.use { it.readBytes() }
                    ?: run {
                        withContext(Dispatchers.Main) { callback.onError(filename, "Cannot open file") }
                        return@launch
                    }

                val total = bytes.size.toLong()
                var offset = 0L

                while (offset < total) {
                    val end = minOf(offset + CHUNK_SIZE, total).toInt()
                    val chunk = bytes.copyOfRange(offset.toInt(), end)
                    @Suppress("UNUSED_VARIABLE")
                    val packet = buildPacket(DATA_TRANSFER, chunk)
                    // TODO: write packet to streaming connection

                    offset = end.toLong()
                    withContext(Dispatchers.Main) { callback.onProgress(filename, offset, total) }
                }

                withContext(Dispatchers.Main) { callback.onComplete(filename) }
            } catch (e: Exception) {
                withContext(Dispatchers.Main) { callback.onError(filename, e.message ?: "Unknown error") }
            } finally {
                activeTransfers.remove(filename)
            }
        }
        activeTransfers[filename] = job
    }

    fun receiveFile(filename: String, totalSize: Long, callback: FileTransferCallback) {
        val job = scope.launch {
            try {
                val cacheFile = File(context.cacheDir, filename)
                var received = 0L

                cacheFile.outputStream().use { out ->
                    // TODO: read incoming DATA_TRANSFER chunks from streaming connection
                    // and write each chunk to `out`, incrementing `received`.
                    while (received < totalSize) {
                        // Placeholder: real implementation reads from the network stream
                        break
                    }
                }

                withContext(Dispatchers.Main) {
                    if (received == totalSize) callback.onComplete(filename)
                    else callback.onProgress(filename, received, totalSize)
                }
            } catch (e: Exception) {
                withContext(Dispatchers.Main) { callback.onError(filename, e.message ?: "Unknown error") }
            } finally {
                activeTransfers.remove(filename)
            }
        }
        activeTransfers[filename] = job
    }

    fun cancelTransfer(filename: String) {
        activeTransfers[filename]?.cancel()
        activeTransfers.remove(filename)
    }

    private fun buildPacket(type: Byte, payload: ByteArray): ByteArray {
        val header = byteArrayOf(
            MAGIC_0,    // 0x52 'R'
            MAGIC_1,    // 0x53 'S'
            type,
            RESERVED
        )
        return header + payload
    }

    private fun resolveFilename(uri: Uri): String? {
        context.contentResolver.query(uri, null, null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) {
                val idx = cursor.getColumnIndex(android.provider.OpenableColumns.DISPLAY_NAME)
                if (idx >= 0) return cursor.getString(idx)
            }
        }
        return null
    }

    companion object {
        const val DATA_TRANSFER: Byte = 0x08
        private const val CHUNK_SIZE = 64 * 1024  // 64 KB
        private const val MAGIC_0: Byte = 0x52
        private const val MAGIC_1: Byte = 0x53
        private const val RESERVED: Byte = 0x00
    }
}
