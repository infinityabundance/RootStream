package com.rootstream.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.rootstream.data.models.Peer
import dagger.hilt.android.lifecycle.HiltViewModel
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import javax.inject.Inject

/**
 * ViewModel for peer discovery
 * Manages mDNS discovery and peer list
 */
@HiltViewModel
class PeerDiscoveryViewModel @Inject constructor(
    // Inject PeerDiscovery service
) : ViewModel() {
    
    private val _peers = MutableStateFlow<List<Peer>>(emptyList())
    val peers: StateFlow<List<Peer>> = _peers.asStateFlow()
    
    private val _isScanning = MutableStateFlow(false)
    val isScanning: StateFlow<Boolean> = _isScanning.asStateFlow()
    
    fun startDiscovery() {
        viewModelScope.launch {
            _isScanning.value = true
            
            // TODO: Integrate with PeerDiscovery service
            delay(2000)
            
            // Mock peers for demonstration
            _peers.value = listOf(
                Peer(
                    name = "Gaming PC",
                    hostname = "gaming-pc.local",
                    port = 48010,
                    ipAddress = "192.168.1.100",
                    capabilities = listOf("H264", "VP9", "Vulkan")
                ),
                Peer(
                    name = "Server",
                    hostname = "server.local",
                    port = 48010,
                    ipAddress = "192.168.1.101",
                    capabilities = listOf("H264", "OpenGL")
                )
            )
            
            _isScanning.value = false
        }
    }
    
    fun refresh() {
        _peers.value = emptyList()
        startDiscovery()
    }
    
    override fun onCleared() {
        super.onCleared()
        // Stop discovery service
    }
}
