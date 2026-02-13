cmake_minimum_required(VERSION 3.22.1)

project("rootstream")

# Vulkan Renderer Native Code
add_library(rootstream_vulkan SHARED
    vulkan_renderer.cpp
)

target_link_libraries(rootstream_vulkan
    android
    log
    vulkan
)

# Opus Decoder Native Code
add_library(rootstream_opus SHARED
    opus_decoder.cpp
)

target_link_libraries(rootstream_opus
    android
    log
    # Link with libopus when available
)

# OpenGL ES utilities (if needed)
add_library(rootstream_gles SHARED
    gles_utils.cpp
)

target_link_libraries(rootstream_gles
    android
    log
    GLESv3
    EGL
)
