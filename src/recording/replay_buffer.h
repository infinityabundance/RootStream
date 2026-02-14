#ifndef REPLAY_BUFFER_H
#define REPLAY_BUFFER_H

#include "recording_types.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_REPLAY_BUFFER_SECONDS 300  // 5 minutes max

typedef struct {
    uint8_t *data;
    size_t size;
    uint64_t timestamp_us;
    uint32_t width;
    uint32_t height;
    bool is_keyframe;
} replay_video_frame_t;

typedef struct {
    float *samples;
    size_t sample_count;
    uint64_t timestamp_us;
    uint32_t sample_rate;
    uint8_t channels;
} replay_audio_chunk_t;

typedef struct replay_buffer replay_buffer_t;

/**
 * Create a new replay buffer
 * 
 * @param duration_seconds  Maximum duration of replay buffer in seconds
 * @param max_memory_mb     Maximum memory to use for buffer (0 = unlimited)
 * @return                  Pointer to replay buffer, or NULL on error
 */
replay_buffer_t* replay_buffer_create(uint32_t duration_seconds, uint32_t max_memory_mb);

/**
 * Add a video frame to the replay buffer
 * 
 * @param buffer       Replay buffer
 * @param frame_data   Raw frame data
 * @param size         Size of frame data in bytes
 * @param width        Frame width
 * @param height       Frame height
 * @param timestamp_us Frame timestamp in microseconds
 * @param is_keyframe  Whether this is a keyframe
 * @return             0 on success, -1 on error
 */
int replay_buffer_add_video_frame(replay_buffer_t *buffer,
                                  const uint8_t *frame_data,
                                  size_t size,
                                  uint32_t width,
                                  uint32_t height,
                                  uint64_t timestamp_us,
                                  bool is_keyframe);

/**
 * Add an audio chunk to the replay buffer
 * 
 * @param buffer       Replay buffer
 * @param samples      Audio sample data
 * @param sample_count Number of samples
 * @param sample_rate  Sample rate in Hz
 * @param channels     Number of audio channels
 * @param timestamp_us Timestamp in microseconds
 * @return             0 on success, -1 on error
 */
int replay_buffer_add_audio_chunk(replay_buffer_t *buffer,
                                  const float *samples,
                                  size_t sample_count,
                                  uint32_t sample_rate,
                                  uint8_t channels,
                                  uint64_t timestamp_us);

/**
 * Save the replay buffer to a file
 * 
 * @param buffer        Replay buffer
 * @param filename      Output filename
 * @param duration_sec  Duration to save (0 = all available)
 * @return              0 on success, -1 on error
 */
int replay_buffer_save(replay_buffer_t *buffer,
                       const char *filename,
                       uint32_t duration_sec);

/**
 * Clear all data from the replay buffer
 * 
 * @param buffer  Replay buffer
 */
void replay_buffer_clear(replay_buffer_t *buffer);

/**
 * Get statistics about the replay buffer
 * 
 * @param buffer           Replay buffer
 * @param video_frames_out Number of video frames stored (output)
 * @param audio_chunks_out Number of audio chunks stored (output)
 * @param memory_used_mb   Memory used in MB (output)
 * @param duration_sec_out Duration available in seconds (output)
 * @return                 0 on success, -1 on error
 */
int replay_buffer_get_stats(replay_buffer_t *buffer,
                           uint32_t *video_frames_out,
                           uint32_t *audio_chunks_out,
                           uint32_t *memory_used_mb,
                           uint32_t *duration_sec_out);

/**
 * Destroy replay buffer and free all resources
 * 
 * @param buffer  Replay buffer to destroy
 */
void replay_buffer_destroy(replay_buffer_t *buffer);

#ifdef __cplusplus
}
#endif

#endif /* REPLAY_BUFFER_H */
