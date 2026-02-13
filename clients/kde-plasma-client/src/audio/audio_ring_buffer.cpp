/* Audio Ring Buffer Implementation */
#include "audio_ring_buffer.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <sys/time.h>

AudioRingBuffer::AudioRingBuffer()
    : buffer(nullptr), buffer_size(0), write_pos(0), read_pos(0),
      sample_rate(0), channels(0), buffer_duration_ms(0),
      underrun_flag(false), overrun_flag(false) {
    pthread_mutex_init(&lock, nullptr);
    pthread_cond_init(&not_empty, nullptr);
    pthread_cond_init(&not_full, nullptr);
}

AudioRingBuffer::~AudioRingBuffer() {
    cleanup();
}

int AudioRingBuffer::init(int sample_rate, int channels, int buffer_duration_ms) {
    if (buffer) {
        cleanup();
    }
    
    this->sample_rate = sample_rate;
    this->channels = channels;
    this->buffer_duration_ms = buffer_duration_ms;
    
    // Calculate buffer size in samples
    buffer_size = (sample_rate * channels * buffer_duration_ms) / 1000;
    
    buffer = (float *)calloc(buffer_size, sizeof(float));
    if (!buffer) {
        fprintf(stderr, "Failed to allocate audio ring buffer\n");
        return -1;
    }
    
    write_pos = 0;
    read_pos = 0;
    underrun_flag = false;
    overrun_flag = false;
    
    return 0;
}

int AudioRingBuffer::write_samples(const float *samples, int sample_count,
                                   int timeout_ms) {
    if (!buffer) {
        return -1;
    }
    
    pthread_mutex_lock(&lock);
    
    // Check if we have enough space
    int free_space = get_free_samples();
    if (sample_count > free_space) {
        if (timeout_ms == 0) {
            pthread_mutex_unlock(&lock);
            overrun_flag = true;
            return -2; // Buffer full
        }
        
        // Wait for space with timeout
        struct timespec ts;
        struct timeval now;
        gettimeofday(&now, nullptr);
        ts.tv_sec = now.tv_sec + (timeout_ms / 1000);
        ts.tv_nsec = (now.tv_usec * 1000) + ((timeout_ms % 1000) * 1000000);
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        
        int result = pthread_cond_timedwait(&not_full, &lock, &ts);
        if (result != 0) {
            pthread_mutex_unlock(&lock);
            overrun_flag = true;
            return -2; // Timeout
        }
        
        free_space = get_free_samples();
        if (sample_count > free_space) {
            pthread_mutex_unlock(&lock);
            overrun_flag = true;
            return -2;
        }
    }
    
    // Write samples
    size_t written = 0;
    while (written < (size_t)sample_count) {
        size_t chunk = sample_count - written;
        size_t space_to_end = buffer_size - write_pos;
        if (chunk > space_to_end) {
            chunk = space_to_end;
        }
        
        memcpy(buffer + write_pos, samples + written, chunk * sizeof(float));
        write_pos = (write_pos + chunk) % buffer_size;
        written += chunk;
    }
    
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&lock);
    
    return written;
}

int AudioRingBuffer::read_samples(float *output, int sample_count,
                                  int timeout_ms) {
    if (!buffer) {
        return -1;
    }
    
    pthread_mutex_lock(&lock);
    
    // Check if we have enough samples
    int available = get_available_samples();
    if (sample_count > available) {
        if (timeout_ms == 0) {
            pthread_mutex_unlock(&lock);
            underrun_flag = true;
            return -2; // Buffer empty
        }
        
        // Wait for data with timeout
        struct timespec ts;
        struct timeval now;
        gettimeofday(&now, nullptr);
        ts.tv_sec = now.tv_sec + (timeout_ms / 1000);
        ts.tv_nsec = (now.tv_usec * 1000) + ((timeout_ms % 1000) * 1000000);
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        
        int result = pthread_cond_timedwait(&not_empty, &lock, &ts);
        if (result != 0) {
            pthread_mutex_unlock(&lock);
            underrun_flag = true;
            return -2; // Timeout
        }
        
        available = get_available_samples();
        if (sample_count > available) {
            pthread_mutex_unlock(&lock);
            underrun_flag = true;
            return -2;
        }
    }
    
    // Read samples
    size_t read = 0;
    while (read < (size_t)sample_count) {
        size_t chunk = sample_count - read;
        size_t data_to_end = buffer_size - read_pos;
        if (chunk > data_to_end) {
            chunk = data_to_end;
        }
        
        memcpy(output + read, buffer + read_pos, chunk * sizeof(float));
        read_pos = (read_pos + chunk) % buffer_size;
        read += chunk;
    }
    
    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&lock);
    
    return read;
}

int AudioRingBuffer::get_available_samples() {
    if (write_pos >= read_pos) {
        return write_pos - read_pos;
    } else {
        return buffer_size - read_pos + write_pos;
    }
}

int AudioRingBuffer::get_free_samples() {
    return buffer_size - get_available_samples() - 1;
}

float AudioRingBuffer::get_fill_percentage() {
    if (buffer_size == 0) return 0.0f;
    return (float)get_available_samples() / (float)buffer_size * 100.0f;
}

int AudioRingBuffer::get_latency_ms() {
    if (sample_rate == 0 || channels == 0) return 0;
    int available = get_available_samples();
    return (available * 1000) / (sample_rate * channels);
}

bool AudioRingBuffer::has_underrun() {
    return underrun_flag;
}

bool AudioRingBuffer::has_overrun() {
    return overrun_flag;
}

void AudioRingBuffer::reset_on_underrun() {
    pthread_mutex_lock(&lock);
    write_pos = 0;
    read_pos = 0;
    underrun_flag = false;
    overrun_flag = false;
    pthread_mutex_unlock(&lock);
}

void AudioRingBuffer::cleanup() {
    if (buffer) {
        free(buffer);
        buffer = nullptr;
    }
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&not_empty);
    pthread_cond_destroy(&not_full);
}
