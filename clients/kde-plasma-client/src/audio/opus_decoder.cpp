/* Opus Decoder Implementation */
#include "opus_decoder.h"
#include <cstring>
#include <cstdio>

OpusDecoderWrapper::OpusDecoderWrapper()
    : decoder(nullptr), sample_rate(0), channels(0), 
      frame_size(0), total_samples_decoded(0) {
}

OpusDecoderWrapper::~OpusDecoderWrapper() {
    cleanup();
}

int OpusDecoderWrapper::init(int sample_rate, int channels) {
    if (decoder) {
        cleanup();
    }
    
    this->sample_rate = sample_rate;
    this->channels = channels;
    
    int error = 0;
    decoder = opus_decoder_create(sample_rate, channels, &error);
    
    if (error != OPUS_OK || !decoder) {
        fprintf(stderr, "Failed to create Opus decoder: %s\n", 
                opus_strerror(error));
        return -1;
    }
    
    // Calculate typical frame size (20ms at given sample rate)
    frame_size = (sample_rate * 20) / 1000;
    
    return 0;
}

int OpusDecoderWrapper::decode_frame(const uint8_t *packet, size_t packet_len,
                                    float *pcm_output, int max_samples) {
    if (!decoder) {
        return -1;
    }
    
    int samples = opus_decode_float(decoder, packet, packet_len, 
                                    pcm_output, max_samples, 0);
    
    if (samples < 0) {
        fprintf(stderr, "Opus decode error: %s\n", opus_strerror(samples));
        return samples;
    }
    
    total_samples_decoded += samples;
    return samples;
}

int OpusDecoderWrapper::decode_frame_with_fec(const uint8_t *packet, 
                                              size_t packet_len,
                                              const uint8_t *fec_packet, 
                                              size_t fec_len,
                                              float *pcm_output, 
                                              int max_samples) {
    if (!decoder) {
        return -1;
    }
    
    // First try to decode with FEC
    int samples = opus_decode_float(decoder, fec_packet, fec_len, 
                                    pcm_output, max_samples, 1);
    
    if (samples < 0) {
        // FEC failed, try regular decode
        samples = decode_frame(packet, packet_len, pcm_output, max_samples);
    } else {
        total_samples_decoded += samples;
    }
    
    return samples;
}

int OpusDecoderWrapper::get_bandwidth() {
    if (!decoder) {
        return -1;
    }
    
    opus_int32 bandwidth;
    int error = opus_decoder_ctl(decoder, OPUS_GET_BANDWIDTH(&bandwidth));
    
    if (error != OPUS_OK) {
        return -1;
    }
    
    return bandwidth;
}

void OpusDecoderWrapper::cleanup() {
    if (decoder) {
        opus_decoder_destroy(decoder);
        decoder = nullptr;
    }
    total_samples_decoded = 0;
}
