#ifndef FRAME_RATE_COUNTER_H
#define FRAME_RATE_COUNTER_H

#include "metrics_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct frame_rate_counter frame_rate_counter_t;

// Initialize frame rate counter
frame_rate_counter_t* frame_rate_counter_init(void);

// Record a frame
void frame_rate_counter_record_frame(frame_rate_counter_t* counter);

// Get current FPS
uint32_t frame_rate_counter_get_fps(frame_rate_counter_t* counter);

// Get current frame time in ms
float frame_rate_counter_get_frame_time_ms(frame_rate_counter_t* counter);

// Get frame statistics
void frame_rate_counter_get_stats(frame_rate_counter_t* counter, frame_rate_metrics_t* out);

// Get dropped frames count
uint32_t frame_rate_counter_get_dropped_frames(frame_rate_counter_t* counter);

// Cleanup
void frame_rate_counter_cleanup(frame_rate_counter_t* counter);

#ifdef __cplusplus
}
#endif

#endif // FRAME_RATE_COUNTER_H
