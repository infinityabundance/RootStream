package com.rootstream.di

import dagger.Module
import dagger.Provides
import dagger.hilt.InstallIn
import dagger.hilt.components.SingletonComponent
import javax.inject.Singleton

/**
 * Hilt module for rendering-related dependencies
 * Provides renderers (Vulkan/OpenGL) and video decoder
 */
@Module
@InstallIn(SingletonComponent::class)
object RenderingModule {
    
    // TODO: Provide VulkanRenderer
    // TODO: Provide OpenGLRenderer
    // TODO: Provide VideoDecoder
}
