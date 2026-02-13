package com.rootstream

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Surface
import androidx.compose.ui.Modifier
import com.rootstream.ui.MainNavigation
import com.rootstream.ui.theme.RootStreamTheme
import dagger.hilt.android.AndroidEntryPoint

/**
 * Main Activity - Entry point for the RootStream Android application
 * Uses Jetpack Compose for UI and Hilt for dependency injection
 */
@AndroidEntryPoint
class MainActivity : ComponentActivity() {
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        setContent {
            RootStreamTheme {
                Surface(
                    modifier = Modifier.fillMaxSize(),
                    color = MaterialTheme.colorScheme.background
                ) {
                    MainNavigation()
                }
            }
        }
    }
}
