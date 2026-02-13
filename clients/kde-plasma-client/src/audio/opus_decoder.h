/* Opus Decoder Wrapper for RootStream */
#ifndef OPUS_DECODER_H
#define OPUS_DECODER_H

#include <opus/opus.h>
#include <stdint.h>
#include <stddef.h>

class OpusDecoderWrapper {
private:
    ::OpusDecoder *decoder;
    int sample_rate;
    int channels;
    int frame_size;
    uint64_t total_samples_decoded;
    
public:
    OpusDecoderWrapper();
    ~OpusDecoderWrapper();
    
    // Initialization
    int init(int sample_rate, int channels);
    
    // Decoding
    int decode_frame(const uint8_t *packet, size_t packet_len,
                    float *pcm_output, int max_samples);
    
    // Error handling with FEC (Forward Error Correction)
    int decode_frame_with_fec(const uint8_t *packet, size_t packet_len,
                             const uint8_t *fec_packet, size_t fec_len,
                             float *pcm_output, int max_samples);
    
    // State queries
    int get_sample_rate() const { return sample_rate; }
    int get_channels() const { return channels; }
    int get_frame_size() const { return frame_size; }
    uint64_t get_total_samples() const { return total_samples_decoded; }
    
    // Bandwidth reporting
    int get_bandwidth();
    
    void cleanup();
};

#endif // OPUS_DECODER_H
