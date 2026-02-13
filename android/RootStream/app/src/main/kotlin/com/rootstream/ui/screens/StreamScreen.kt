package com.rootstream.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import com.rootstream.R
import com.rootstream.data.models.ConnectionState
import com.rootstream.ui.components.StatusOverlay
import com.rootstream.viewmodel.StreamViewModel

/**
 * Stream screen
 * Main streaming view with video rendering and controls
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun StreamScreen(
    peerId: String,
    onDisconnect: () -> Unit,
    viewModel: StreamViewModel = hiltViewModel()
) {
    val connectionState by viewModel.connectionState.collectAsState()
    val stats by viewModel.stats.collectAsState()
    
    LaunchedEffect(peerId) {
        viewModel.connect(peerId)
    }
    
    DisposableEffect(Unit) {
        onDispose {
            viewModel.disconnect()
        }
    }
    
    Box(modifier = Modifier.fillMaxSize()) {
        // Rendering surface will be integrated here
        Box(
            modifier = Modifier
                .fillMaxSize()
                .background(Color.Black),
            contentAlignment = Alignment.Center
        ) {
            when (connectionState) {
                ConnectionState.CONNECTING -> {
                    CircularProgressIndicator(color = Color.White)
                    Text(
                        text = stringResource(R.string.stream_connecting),
                        color = Color.White,
                        modifier = Modifier.padding(top = 80.dp)
                    )
                }
                ConnectionState.DISCONNECTED -> {
                    Text(
                        text = stringResource(R.string.stream_disconnected),
                        color = Color.White
                    )
                }
                ConnectionState.ERROR -> {
                    Text(
                        text = stringResource(R.string.stream_error),
                        color = Color.Red
                    )
                }
                else -> {
                    // Streaming - render surface active
                }
            }
        }
        
        // Status overlay
        if (connectionState == ConnectionState.STREAMING) {
            StatusOverlay(
                fps = stats.fps,
                latency = stats.latency,
                modifier = Modifier
                    .align(Alignment.TopEnd)
                    .padding(16.dp)
            )
        }
        
        // Back button
        IconButton(
            onClick = onDisconnect,
            modifier = Modifier
                .align(Alignment.TopStart)
                .padding(16.dp)
        ) {
            Icon(
                Icons.Default.ArrowBack,
                contentDescription = "Disconnect",
                tint = Color.White
            )
        }
    }
}
