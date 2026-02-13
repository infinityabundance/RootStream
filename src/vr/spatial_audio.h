#ifndef SPATIAL_AUDIO_H
#define SPATIAL_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "openxr_manager.h"

// Audio source structure
typedef struct {
    uint32_t sourceId;
    XrVector3f position;
    XrVector3f velocity;
    float radius;
    float volume;
    bool isHeadRelative;
    bool active;
} AudioSource;

// Spatial audio engine structure
typedef struct SpatialAudioEngine SpatialAudioEngine;

// Creation and initialization
SpatialAudioEngine* spatial_audio_engine_create(void);
int spatial_audio_engine_init(SpatialAudioEngine *engine);
void spatial_audio_engine_cleanup(SpatialAudioEngine *engine);
void spatial_audio_engine_destroy(SpatialAudioEngine *engine);

// Audio source management
uint32_t spatial_audio_create_source(SpatialAudioEngine *engine, 
                                     const XrVector3f *position, float radius);
int spatial_audio_update_source_position(SpatialAudioEngine *engine, 
                                         uint32_t sourceId, const XrVector3f *position);
int spatial_audio_set_source_volume(SpatialAudioEngine *engine, 
                                    uint32_t sourceId, float volume);
int spatial_audio_destroy_source(SpatialAudioEngine *engine, uint32_t sourceId);

// HRTF processing (Head-Related Transfer Function)
int spatial_audio_apply_hrtf(SpatialAudioEngine *engine, uint32_t sourceId,
                             const uint8_t *audioData, size_t dataSize,
                             uint8_t *processedData);

// Head-relative audio
int spatial_audio_process_head_relative(SpatialAudioEngine *engine, uint32_t sourceId,
                                       const uint8_t *audioData, size_t dataSize,
                                       const XrQuaternionf *headOrientation,
                                       uint8_t *processedData);

// Update listener position (head position)
int spatial_audio_update_listener(SpatialAudioEngine *engine, 
                                  const XrVector3f *position,
                                  const XrQuaternionf *orientation);

#ifdef __cplusplus
}
#endif

#endif // SPATIAL_AUDIO_H
