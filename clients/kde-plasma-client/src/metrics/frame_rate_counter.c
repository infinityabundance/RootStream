#include "frame_rate_counter.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct frame_rate_counter {
    uint64_t frame_timestamps[METRICS_HISTORY_SIZE];
    uint32_t frame_index;
    uint32_t total_frames;
    uint64_t window_start_time_us;
    uint64_t last_frame_time_us;
    uint32_t frame_drops;
    float expected_frame_time_ms;
};

static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

frame_rate_counter_t* frame_rate_counter_init(void) {
    frame_rate_counter_t* counter = (frame_rate_counter_t*)calloc(1, sizeof(frame_rate_counter_t));
    if (!counter) {
        return NULL;
    }
    
    counter->window_start_time_us = get_time_us();
    counter->last_frame_time_us = counter->window_start_time_us;
    counter->expected_frame_time_ms = 16.67f; // 60 FPS target
    
    return counter;
}

void frame_rate_counter_record_frame(frame_rate_counter_t* counter) {
    if (!counter) return;
    
    uint64_t now = get_time_us();
    counter->frame_timestamps[counter->frame_index] = now;
    counter->frame_index = (counter->frame_index + 1) % METRICS_HISTORY_SIZE;
    counter->total_frames++;
    
    // Detect dropped frames (if frame time > 1.5x expected)
    if (counter->last_frame_time_us > 0) {
        float frame_time_ms = (now - counter->last_frame_time_us) / 1000.0f;
        if (frame_time_ms > counter->expected_frame_time_ms * 1.5f) {
            counter->frame_drops++;
        }
    }
    
    counter->last_frame_time_us = now;
}

uint32_t frame_rate_counter_get_fps(frame_rate_counter_t* counter) {
    if (!counter || counter->total_frames == 0) return 0;
    
    uint64_t now = get_time_us();
    uint32_t samples = counter->total_frames < METRICS_HISTORY_SIZE ? 
                      counter->total_frames : METRICS_HISTORY_SIZE;
    
    if (samples < 2) return 0;
    
    // Get oldest timestamp in window
    uint32_t oldest_idx = (counter->frame_index + METRICS_HISTORY_SIZE - samples) % METRICS_HISTORY_SIZE;
    uint64_t oldest_time = counter->frame_timestamps[oldest_idx];
    
    if (oldest_time == 0) return 0;
    
    uint64_t time_span_us = now - oldest_time;
    if (time_span_us == 0) return 0;
    
    return (uint32_t)((samples - 1) * 1000000ULL / time_span_us);
}

float frame_rate_counter_get_frame_time_ms(frame_rate_counter_t* counter) {
    if (!counter || counter->total_frames < 2) return 0.0f;
    
    uint32_t prev_idx = (counter->frame_index + METRICS_HISTORY_SIZE - 2) % METRICS_HISTORY_SIZE;
    uint32_t last_idx = (counter->frame_index + METRICS_HISTORY_SIZE - 1) % METRICS_HISTORY_SIZE;
    
    if (counter->frame_timestamps[last_idx] == 0 || counter->frame_timestamps[prev_idx] == 0) {
        return 0.0f;
    }
    
    uint64_t delta = counter->frame_timestamps[last_idx] - counter->frame_timestamps[prev_idx];
    return delta / 1000.0f;
}

void frame_rate_counter_get_stats(frame_rate_counter_t* counter, frame_rate_metrics_t* out) {
    if (!counter || !out) return;
    
    memset(out, 0, sizeof(frame_rate_metrics_t));
    
    out->fps = frame_rate_counter_get_fps(counter);
    out->frame_time_ms = frame_rate_counter_get_frame_time_ms(counter);
    out->frame_drops = counter->frame_drops;
    out->total_frames = counter->total_frames;
    
    // Calculate min/max/avg frame times
    uint32_t samples = counter->total_frames < METRICS_HISTORY_SIZE ? 
                      counter->total_frames : METRICS_HISTORY_SIZE;
    
    if (samples < 2) return;
    
    float sum = 0.0f;
    out->min_frame_time_ms = 999999.0f;
    out->max_frame_time_ms = 0.0f;
    uint32_t count = 0;
    
    for (uint32_t i = 1; i < samples; i++) {
        uint32_t curr_idx = (counter->frame_index + METRICS_HISTORY_SIZE - i) % METRICS_HISTORY_SIZE;
        uint32_t prev_idx = (counter->frame_index + METRICS_HISTORY_SIZE - i - 1) % METRICS_HISTORY_SIZE;
        
        if (counter->frame_timestamps[curr_idx] == 0 || counter->frame_timestamps[prev_idx] == 0) {
            continue;
        }
        
        float frame_time = (counter->frame_timestamps[curr_idx] - counter->frame_timestamps[prev_idx]) / 1000.0f;
        
        if (frame_time < out->min_frame_time_ms) out->min_frame_time_ms = frame_time;
        if (frame_time > out->max_frame_time_ms) out->max_frame_time_ms = frame_time;
        sum += frame_time;
        count++;
    }
    
    if (count > 0) {
        out->avg_frame_time_ms = sum / count;
    }
    
    // Reset min if no valid samples
    if (out->min_frame_time_ms == 999999.0f) {
        out->min_frame_time_ms = 0.0f;
    }
}

uint32_t frame_rate_counter_get_dropped_frames(frame_rate_counter_t* counter) {
    return counter ? counter->frame_drops : 0;
}

void frame_rate_counter_cleanup(frame_rate_counter_t* counter) {
    if (counter) {
        free(counter);
    }
}
