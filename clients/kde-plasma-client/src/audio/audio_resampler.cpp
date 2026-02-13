/* Audio Resampler Implementation */
#include "audio_resampler.h"
#include <cstdio>
#include <cstring>

AudioResampler::AudioResampler()
    : src_state(nullptr), src_quality(SRC_SINC_MEDIUM_QUALITY),
      input_rate(0), output_rate(0), channels(0), conversion_ratio(1.0f) {
}

AudioResampler::~AudioResampler() {
    cleanup();
}

int AudioResampler::init(int input_rate, int output_rate, int channels,
                        int quality) {
    if (src_state) {
        cleanup();
    }
    
    this->input_rate = input_rate;
    this->output_rate = output_rate;
    this->channels = channels;
    this->src_quality = quality;
    
    // Calculate conversion ratio
    conversion_ratio = (float)output_rate / (float)input_rate;
    
    int error = 0;
    src_state = src_new(quality, channels, &error);
    
    if (!src_state || error != 0) {
        fprintf(stderr, "Failed to create resampler: %s\n", 
                src_strerror(error));
        return -1;
    }
    
    return 0;
}

int AudioResampler::resample(const float *input, int input_frames,
                             float *output, int *output_frames) {
    if (!src_state) {
        return -1;
    }
    
    SRC_DATA src_data;
    memset(&src_data, 0, sizeof(src_data));
    
    src_data.data_in = input;
    src_data.input_frames = input_frames;
    src_data.data_out = output;
    src_data.output_frames = *output_frames;
    src_data.src_ratio = conversion_ratio;
    src_data.end_of_input = 0;
    
    int error = src_process(src_state, &src_data);
    
    if (error != 0) {
        fprintf(stderr, "Resampling error: %s\n", src_strerror(error));
        return -1;
    }
    
    *output_frames = src_data.output_frames_gen;
    return 0;
}

int AudioResampler::set_output_rate(int new_rate) {
    if (new_rate == output_rate) {
        return 0;
    }
    
    output_rate = new_rate;
    conversion_ratio = (float)output_rate / (float)input_rate;
    
    // Reset the resampler state
    if (src_state) {
        int error = src_reset(src_state);
        if (error != 0) {
            fprintf(stderr, "Failed to reset resampler: %s\n", 
                    src_strerror(error));
            return -1;
        }
    }
    
    return 0;
}

void AudioResampler::cleanup() {
    if (src_state) {
        src_delete(src_state);
        src_state = nullptr;
    }
}
