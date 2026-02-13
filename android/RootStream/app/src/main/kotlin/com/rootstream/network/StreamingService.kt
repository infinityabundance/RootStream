package com.rootstream.network

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import com.rootstream.R

/**
 * Foreground service for background streaming
 * Keeps streaming active when app is in background
 * 
 * TODO Phase 22.2.13: Implement with:
 * - Notification controls (play/pause/stop)
 * - Lifecycle management for PiP transitions
 * - Proper cleanup on service stop
 */
class StreamingService : Service() {
    
    companion object {
        private const val CHANNEL_ID = "streaming_channel"
        private const val NOTIFICATION_ID = 1
    }
    
    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
    }
    
    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        val notification = createNotification()
        startForeground(NOTIFICATION_ID, notification)
        
        // TODO: Initialize streaming components
        
        return START_STICKY
    }
    
    override fun onBind(intent: Intent?): IBinder? = null
    
    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                getString(R.string.notification_channel_name),
                NotificationManager.IMPORTANCE_LOW
            ).apply {
                description = getString(R.string.notification_channel_description)
            }
            
            val notificationManager = getSystemService(NotificationManager::class.java)
            notificationManager.createNotificationChannel(channel)
        }
    }
    
    private fun createNotification(): Notification {
        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle(getString(R.string.notification_streaming_title))
            .setContentText(getString(R.string.notification_streaming_text, "Host"))
            .setSmallIcon(R.drawable.ic_launcher_foreground)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build()
    }
    
    override fun onDestroy() {
        super.onDestroy()
        // TODO: Cleanup streaming resources
    }
}
