/* Audio Resampler Wrapper for RootStream */
#ifndef AUDIO_RESAMPLER_H
#define AUDIO_RESAMPLER_H

#include <samplerate.h>
#include <stdint.h>

class AudioResampler {
private:
    SRC_STATE *src_state;
    int src_quality;
    int input_rate;
    int output_rate;
    int channels;
    float conversion_ratio;
    
public:
    AudioResampler();
    ~AudioResampler();
    
    // Initialization
    int init(int input_rate, int output_rate, int channels,
            int quality = SRC_SINC_MEDIUM_QUALITY);
    
    // Resampling
    int resample(const float *input, int input_frames,
                float *output, int *output_frames);
    
    // Rate changes
    int set_output_rate(int new_rate);
    
    // State queries
    int get_input_rate() const { return input_rate; }
    int get_output_rate() const { return output_rate; }
    int get_channels() const { return channels; }
    float get_ratio() const { return conversion_ratio; }
    
    void cleanup();
};

#endif // AUDIO_RESAMPLER_H
