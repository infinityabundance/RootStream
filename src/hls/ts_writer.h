/*
 * ts_writer.h — Minimal MPEG-TS segment writer
 *
 * Writes raw NAL-unit payloads (H.264/H.265) into MPEG-TS packets
 * suitable for HLS segments.  Only the subset of MPEG-TS required
 * for single-program HLS is implemented:
 *
 *   PAT  — Program Association Table  (PID 0)
 *   PMT  — Program Map Table          (PID 4096)
 *   PES  — Packetized Elementary Stream (video PID 256)
 *
 * No audio muxing: a video-only TS is sufficient for the HLS tests.
 * Audio can be added as an additional PES stream in a future microtask.
 *
 * Thread-safety: NOT thread-safe; use one ts_writer_t per encoding thread.
 */

#ifndef ROOTSTREAM_TS_WRITER_H
#define ROOTSTREAM_TS_WRITER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hls_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque TS writer handle */
typedef struct ts_writer_s ts_writer_t;

/**
 * ts_writer_create — allocate a TS writer writing to @fd
 *
 * @param fd       Open writable file descriptor (segment file)
 * @return         Non-NULL handle, or NULL on OOM
 */
ts_writer_t *ts_writer_create(int fd);

/**
 * ts_writer_destroy — free writer (does NOT close @fd)
 *
 * @param w  Writer to destroy
 */
void ts_writer_destroy(ts_writer_t *w);

/**
 * ts_writer_write_pat_pmt — write PAT + PMT tables
 *
 * Must be called once at the start of each segment so decoders can
 * locate the program structure.
 *
 * @param w  TS writer
 * @return   0 on success, -1 on write error
 */
int ts_writer_write_pat_pmt(ts_writer_t *w);

/**
 * ts_writer_write_pes — wrap @payload bytes as a PES packet and write TS
 *
 * Splits the PES into 188-byte TS packets.  Sets the random-access
 * indicator on the first packet when @is_keyframe is true.
 *
 * @param w           TS writer
 * @param payload     NAL unit data
 * @param payload_len Size in bytes
 * @param pts_90khz   Presentation timestamp in 90 kHz units
 * @param is_keyframe True if this is an IDR / keyframe
 * @return            0 on success, -1 on error
 */
int ts_writer_write_pes(ts_writer_t *w, const uint8_t *payload, size_t payload_len,
                        uint64_t pts_90khz, bool is_keyframe);

/**
 * ts_writer_bytes_written — total bytes written so far
 *
 * @param w  TS writer
 * @return   Byte count
 */
size_t ts_writer_bytes_written(const ts_writer_t *w);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_TS_WRITER_H */
