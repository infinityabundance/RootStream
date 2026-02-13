package com.rootstream.ui.screens

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.unit.dp
import androidx.hilt.navigation.compose.hiltViewModel
import com.rootstream.R
import com.rootstream.viewmodel.SettingsViewModel

/**
 * Settings screen
 * Configure video, audio, input, and network settings
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SettingsScreen(
    onNavigateBack: () -> Unit,
    viewModel: SettingsViewModel = hiltViewModel()
) {
    val settings by viewModel.settings.collectAsState()
    
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text(stringResource(R.string.settings_title)) },
                navigationIcon = {
                    IconButton(onClick = onNavigateBack) {
                        Icon(Icons.Default.ArrowBack, contentDescription = "Back")
                    }
                }
            )
        }
    ) { paddingValues ->
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(paddingValues),
            contentPadding = PaddingValues(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            // Video Settings
            item {
                Text(
                    text = stringResource(R.string.settings_video),
                    style = MaterialTheme.typography.titleMedium,
                    color = MaterialTheme.colorScheme.primary
                )
            }
            
            item {
                var selectedRenderer by remember { mutableStateOf(settings.renderer) }
                Column {
                    Text(stringResource(R.string.settings_renderer))
                    Row {
                        RadioButton(
                            selected = selectedRenderer == "Vulkan",
                            onClick = { 
                                selectedRenderer = "Vulkan"
                                viewModel.updateRenderer("Vulkan")
                            }
                        )
                        Text(
                            stringResource(R.string.settings_renderer_vulkan),
                            modifier = Modifier.padding(start = 8.dp)
                        )
                    }
                    Row {
                        RadioButton(
                            selected = selectedRenderer == "OpenGL",
                            onClick = { 
                                selectedRenderer = "OpenGL"
                                viewModel.updateRenderer("OpenGL")
                            }
                        )
                        Text(
                            stringResource(R.string.settings_renderer_opengl),
                            modifier = Modifier.padding(start = 8.dp)
                        )
                    }
                }
            }
            
            // Audio Settings
            item {
                Divider()
                Text(
                    text = stringResource(R.string.settings_audio),
                    style = MaterialTheme.typography.titleMedium,
                    color = MaterialTheme.colorScheme.primary
                )
            }
            
            item {
                var audioEnabled by remember { mutableStateOf(settings.audioEnabled) }
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text(stringResource(R.string.settings_audio_enabled))
                    Switch(
                        checked = audioEnabled,
                        onCheckedChange = { 
                            audioEnabled = it
                            viewModel.updateAudioEnabled(it)
                        }
                    )
                }
            }
            
            // Input Settings
            item {
                Divider()
                Text(
                    text = stringResource(R.string.settings_input),
                    style = MaterialTheme.typography.titleMedium,
                    color = MaterialTheme.colorScheme.primary
                )
            }
            
            item {
                var hapticEnabled by remember { mutableStateOf(settings.hapticFeedback) }
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text(stringResource(R.string.settings_haptic_feedback))
                    Switch(
                        checked = hapticEnabled,
                        onCheckedChange = { 
                            hapticEnabled = it
                            viewModel.updateHapticFeedback(it)
                        }
                    )
                }
            }
            
            // Battery Optimization
            item {
                Divider()
                var batteryOptimization by remember { mutableStateOf(settings.batteryOptimization) }
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text(stringResource(R.string.settings_battery_optimization))
                    Switch(
                        checked = batteryOptimization,
                        onCheckedChange = { 
                            batteryOptimization = it
                            viewModel.updateBatteryOptimization(it)
                        }
                    )
                }
            }
        }
    }
}

data class AppSettings(
    val renderer: String = "Vulkan",
    val audioEnabled: Boolean = true,
    val hapticFeedback: Boolean = true,
    val batteryOptimization: Boolean = true
)
