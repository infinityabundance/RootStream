package com.rootstream

import android.app.Application
import dagger.hilt.android.HiltAndroidApp

/**
 * RootStream Application class
 * Entry point for the Android application with Hilt DI
 */
@HiltAndroidApp
class RootStreamApplication : Application() {
    
    override fun onCreate() {
        super.onCreate()
        
        // Initialize any application-wide components here
        // Security, logging, crash reporting, etc.
    }
}
