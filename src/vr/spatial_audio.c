#include "spatial_audio.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define MAX_AUDIO_SOURCES 64

struct SpatialAudioEngine {
    AudioSource sources[MAX_AUDIO_SOURCES];
    uint32_t sourceCount;
    uint32_t nextSourceId;
    
    XrVector3f listenerPosition;
    XrQuaternionf listenerOrientation;
    
    bool initialized;
};

SpatialAudioEngine* spatial_audio_engine_create(void) {
    SpatialAudioEngine *engine = (SpatialAudioEngine*)calloc(1, sizeof(SpatialAudioEngine));
    if (!engine) {
        fprintf(stderr, "Failed to allocate SpatialAudioEngine\n");
        return NULL;
    }
    
    engine->initialized = false;
    engine->sourceCount = 0;
    engine->nextSourceId = 1;
    
    return engine;
}

int spatial_audio_engine_init(SpatialAudioEngine *engine) {
    if (!engine) {
        return -1;
    }
    
    memset(engine->sources, 0, sizeof(engine->sources));
    
    engine->listenerPosition.x = 0.0f;
    engine->listenerPosition.y = 0.0f;
    engine->listenerPosition.z = 0.0f;
    
    engine->listenerOrientation.w = 1.0f;
    engine->listenerOrientation.x = 0.0f;
    engine->listenerOrientation.y = 0.0f;
    engine->listenerOrientation.z = 0.0f;
    
    engine->initialized = true;
    
    printf("Spatial audio engine initialized\n");
    
    return 0;
}

uint32_t spatial_audio_create_source(SpatialAudioEngine *engine, 
                                     const XrVector3f *position, float radius) {
    if (!engine || !engine->initialized || engine->sourceCount >= MAX_AUDIO_SOURCES) {
        return 0;
    }
    
    uint32_t sourceId = engine->nextSourceId++;
    
    for (uint32_t i = 0; i < MAX_AUDIO_SOURCES; i++) {
        if (!engine->sources[i].active) {
            engine->sources[i].sourceId = sourceId;
            engine->sources[i].position = *position;
            engine->sources[i].radius = radius;
            engine->sources[i].volume = 1.0f;
            engine->sources[i].active = true;
            engine->sources[i].isHeadRelative = false;
            engine->sourceCount++;
            
            return sourceId;
        }
    }
    
    return 0;
}

int spatial_audio_update_source_position(SpatialAudioEngine *engine, 
                                         uint32_t sourceId, const XrVector3f *position) {
    if (!engine || !engine->initialized || !position) {
        return -1;
    }
    
    for (uint32_t i = 0; i < MAX_AUDIO_SOURCES; i++) {
        if (engine->sources[i].active && engine->sources[i].sourceId == sourceId) {
            engine->sources[i].position = *position;
            return 0;
        }
    }
    
    return -1;
}

int spatial_audio_set_source_volume(SpatialAudioEngine *engine, 
                                    uint32_t sourceId, float volume) {
    if (!engine || !engine->initialized) {
        return -1;
    }
    
    for (uint32_t i = 0; i < MAX_AUDIO_SOURCES; i++) {
        if (engine->sources[i].active && engine->sources[i].sourceId == sourceId) {
            engine->sources[i].volume = volume;
            return 0;
        }
    }
    
    return -1;
}

int spatial_audio_destroy_source(SpatialAudioEngine *engine, uint32_t sourceId) {
    if (!engine || !engine->initialized) {
        return -1;
    }
    
    for (uint32_t i = 0; i < MAX_AUDIO_SOURCES; i++) {
        if (engine->sources[i].active && engine->sources[i].sourceId == sourceId) {
            engine->sources[i].active = false;
            engine->sourceCount--;
            return 0;
        }
    }
    
    return -1;
}

int spatial_audio_apply_hrtf(SpatialAudioEngine *engine, uint32_t sourceId,
                             const uint8_t *audioData, size_t dataSize,
                             uint8_t *processedData) {
    if (!engine || !engine->initialized || !audioData || !processedData) {
        return -1;
    }
    
    // Find the source
    AudioSource *source = NULL;
    for (uint32_t i = 0; i < MAX_AUDIO_SOURCES; i++) {
        if (engine->sources[i].active && engine->sources[i].sourceId == sourceId) {
            source = &engine->sources[i];
            break;
        }
    }
    
    if (!source) {
        return -1;
    }
    
    // In a real implementation, would:
    // 1. Calculate direction from listener to source
    // 2. Apply HRTF filters based on direction
    // 3. Apply distance attenuation
    // 4. Apply doppler effect if source has velocity
    
    // For now, simple copy
    memcpy(processedData, audioData, dataSize);
    
    return 0;
}

int spatial_audio_process_head_relative(SpatialAudioEngine *engine, uint32_t sourceId,
                                       const uint8_t *audioData, size_t dataSize,
                                       const XrQuaternionf *headOrientation,
                                       uint8_t *processedData) {
    if (!engine || !engine->initialized || !audioData || !processedData || !headOrientation) {
        return -1;
    }
    
    // In a real implementation, would apply head rotation to audio positioning
    
    return spatial_audio_apply_hrtf(engine, sourceId, audioData, dataSize, processedData);
}

int spatial_audio_update_listener(SpatialAudioEngine *engine, 
                                  const XrVector3f *position,
                                  const XrQuaternionf *orientation) {
    if (!engine || !engine->initialized || !position || !orientation) {
        return -1;
    }
    
    engine->listenerPosition = *position;
    engine->listenerOrientation = *orientation;
    
    return 0;
}

void spatial_audio_engine_cleanup(SpatialAudioEngine *engine) {
    if (!engine) {
        return;
    }
    
    engine->initialized = false;
    engine->sourceCount = 0;
    
    printf("Spatial audio engine cleaned up\n");
}

void spatial_audio_engine_destroy(SpatialAudioEngine *engine) {
    if (!engine) {
        return;
    }
    
    spatial_audio_engine_cleanup(engine);
    free(engine);
}
