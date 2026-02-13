package com.rootstream.network

import android.content.Context
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import com.rootstream.data.models.Peer
import dagger.hilt.android.qualifiers.ApplicationContext
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.callbackFlow
import javax.inject.Inject
import javax.inject.Singleton

/**
 * mDNS peer discovery using NsdManager
 * Discovers RootStream hosts on the local network
 * 
 * TODO Phase 22.2.10: Complete implementation with:
 * - Service registration listeners
 * - Endpoint resolution
 * - Network change detection
 * - Automatic peer refresh (60s timeout)
 * - Cleanup stale entries
 */
@Singleton
class PeerDiscovery @Inject constructor(
    @ApplicationContext private val context: Context
) {
    
    private val nsdManager: NsdManager by lazy {
        context.getSystemService(Context.NSD_SERVICE) as NsdManager
    }
    
    private val serviceType = "_rootstream._tcp."
    
    fun discoverPeers(): Flow<Peer> = callbackFlow {
        val discoveryListener = object : NsdManager.DiscoveryListener {
            override fun onStartDiscoveryFailed(serviceType: String?, errorCode: Int) {
                // TODO: Handle discovery failure
            }
            
            override fun onStopDiscoveryFailed(serviceType: String?, errorCode: Int) {
                // TODO: Handle stop failure
            }
            
            override fun onDiscoveryStarted(serviceType: String?) {
                // Discovery started successfully
            }
            
            override fun onDiscoveryStopped(serviceType: String?) {
                // Discovery stopped
            }
            
            override fun onServiceFound(serviceInfo: NsdServiceInfo?) {
                serviceInfo?.let { info ->
                    // Resolve the service to get IP address and port
                    resolveService(info)
                }
            }
            
            override fun onServiceLost(serviceInfo: NsdServiceInfo?) {
                // TODO: Remove peer from list
            }
        }
        
        // Start discovery
        nsdManager.discoverServices(serviceType, NsdManager.PROTOCOL_DNS_SD, discoveryListener)
        
        awaitClose {
            nsdManager.stopServiceDiscovery(discoveryListener)
        }
    }
    
    private fun resolveService(serviceInfo: NsdServiceInfo) {
        val resolveListener = object : NsdManager.ResolveListener {
            override fun onResolveFailed(serviceInfo: NsdServiceInfo?, errorCode: Int) {
                // TODO: Handle resolve failure
            }
            
            override fun onServiceResolved(serviceInfo: NsdServiceInfo?) {
                serviceInfo?.let { info ->
                    // TODO: Create Peer object and emit to flow
                    val peer = Peer(
                        name = info.serviceName,
                        hostname = info.host.hostName ?: "",
                        port = info.port,
                        ipAddress = info.host.hostAddress ?: ""
                    )
                }
            }
        }
        
        nsdManager.resolveService(serviceInfo, resolveListener)
    }
    
    fun stopDiscovery() {
        // Discovery will be stopped when flow is closed
    }
}
