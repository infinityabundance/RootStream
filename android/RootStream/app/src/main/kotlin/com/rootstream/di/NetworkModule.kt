package com.rootstream.di

import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.components.SingletonComponent
import javax.inject.Singleton

/**
 * Hilt module for network-related dependencies
 * Provides StreamingClient and PeerDiscovery services
 */
@Module
@InstallIn(SingletonComponent::class)
object NetworkModule {
    
    // TODO: Provide StreamingClient
    // TODO: Provide PeerDiscovery
    // TODO: Provide OkHttpClient with TLS configuration
}
