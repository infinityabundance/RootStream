/*
 * plc_frame.h — PCM audio frame format for Packet Loss Concealment
 *
 * A plc_frame_t is a fixed-size block of interleaved 16-bit PCM samples
 * together with the metadata required for concealment decisions.
 *
 * Wire encoding (little-endian)
 * ─────────────────────────────
 *  Offset  Size  Field
 *   0      4     Magic       0x504C4346 ('PLCF')
 *   4      8     timestamp_us — capture time (µs epoch)
 *  12      4     seq_num     — stream sequence number
 *  16      4     sample_rate — Hz (e.g. 48000)
 *  20      2     channels    — channel count (1 or 2)
 *  22      2     num_samples — samples per channel
 *  24      N     samples     — interleaved int16_t, N = channels * num_samples * 2 bytes
 *
 * Maximum frame: PLC_MAX_CHANNELS * PLC_MAX_SAMPLES_PER_CH samples.
 */

#ifndef ROOTSTREAM_PLC_FRAME_H
#define ROOTSTREAM_PLC_FRAME_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLC_FRAME_MAGIC           0x504C4346UL  /* 'PLCF' */
#define PLC_FRAME_HDR_SIZE        24
#define PLC_MAX_CHANNELS          2
#define PLC_MAX_SAMPLES_PER_CH    1024          /* ~21ms at 48 kHz */
#define PLC_MAX_FRAME_SAMPLES     (PLC_MAX_CHANNELS * PLC_MAX_SAMPLES_PER_CH)

/** PCM audio frame */
typedef struct {
    uint64_t timestamp_us;
    uint32_t seq_num;
    uint32_t sample_rate;
    uint16_t channels;
    uint16_t num_samples;          /**< Samples per channel */
    int16_t  samples[PLC_MAX_FRAME_SAMPLES]; /**< Interleaved */
} plc_frame_t;

/**
 * plc_frame_encode — serialise @frame into @buf
 *
 * @param frame   Frame to encode
 * @param buf     Output buffer
 * @param buf_sz  Buffer size
 * @return        Bytes written, or -1 on error
 */
int plc_frame_encode(const plc_frame_t *frame,
                      uint8_t           *buf,
                      size_t             buf_sz);

/**
 * plc_frame_decode — parse @frame from @buf
 *
 * @param buf     Input buffer
 * @param buf_sz  Valid bytes in @buf
 * @param frame   Output frame
 * @return        0 on success, -1 on error
 */
int plc_frame_decode(const uint8_t *buf,
                      size_t         buf_sz,
                      plc_frame_t   *frame);

/**
 * plc_frame_byte_size — total encoded size for a given frame
 *
 * @param frame  Frame
 * @return       Total bytes (header + sample data), or -1 on NULL
 */
int plc_frame_byte_size(const plc_frame_t *frame);

/**
 * plc_frame_is_silent — return true if all samples are zero
 *
 * @param frame  Frame
 * @return       true if silent
 */
bool plc_frame_is_silent(const plc_frame_t *frame);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_PLC_FRAME_H */
