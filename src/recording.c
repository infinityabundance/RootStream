/*
 * recording.c - Stream recording to file
 *
 * Simple container format for saving RootStream sessions.
 * Stores video frames with timestamps for playback.
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

/* RootStream Recording Format (RSTR) */
#define RSTR_MAGIC 0x52535452  /* "RSTR" */
#define RSTR_VERSION 1

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint32_t width;
    uint32_t height;
    uint32_t codec;        /* 0=H.264, 1=H.265 */
    uint32_t fps;
    uint64_t start_time;   /* Unix timestamp */
    uint32_t reserved[8];  /* For future use */
} rstr_header_t;

typedef struct __attribute__((packed)) {
    uint64_t timestamp_us; /* Relative to start */
    uint32_t size;
    uint8_t flags;         /* 0x01=keyframe */
    uint8_t reserved[3];
} rstr_frame_header_t;

/*
 * Initialize recording to file
 */
int recording_init(rootstream_ctx_t *ctx, const char *filename) {
    if (!ctx || !filename) {
        fprintf(stderr, "ERROR: Invalid recording parameters\n");
        return -1;
    }

    /* Open file for writing */
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Cannot create recording file: %s\n", strerror(errno));
        return -1;
    }

    /* Write header */
    rstr_header_t header = {0};
    header.magic = RSTR_MAGIC;
    header.version = RSTR_VERSION;
    header.width = ctx->display.width;
    header.height = ctx->display.height;
    header.codec = (ctx->encoder.codec == CODEC_H265) ? 1 : 0;
    header.fps = ctx->display.refresh_rate;
    header.start_time = (uint64_t)time(NULL);

    if (write(fd, &header, sizeof(header)) != sizeof(header)) {
        fprintf(stderr, "ERROR: Failed to write recording header: %s\n", strerror(errno));
        close(fd);
        unlink(filename);
        return -1;
    }

    ctx->recording.fd = fd;
    ctx->recording.active = true;
    ctx->recording.frame_count = 0;
    ctx->recording.start_time_us = get_timestamp_us();
    strncpy(ctx->recording.filename, filename, sizeof(ctx->recording.filename) - 1);
    ctx->recording.filename[sizeof(ctx->recording.filename) - 1] = '\0';

    printf("✓ Recording started: %s (%dx%d @ %d fps, %s)\n",
           filename, header.width, header.height, header.fps,
           header.codec == 1 ? "H.265" : "H.264");

    return 0;
}

/*
 * Write encoded frame to recording file
 */
int recording_write_frame(rootstream_ctx_t *ctx, const uint8_t *data,
                          size_t size, bool is_keyframe) {
    if (!ctx || !ctx->recording.active || !data || size == 0) {
        return -1;
    }

    /* Write frame header */
    rstr_frame_header_t frame_hdr = {0};
    frame_hdr.timestamp_us = get_timestamp_us() - ctx->recording.start_time_us;
    frame_hdr.size = (uint32_t)size;
    frame_hdr.flags = is_keyframe ? 0x01 : 0x00;

    if (write(ctx->recording.fd, &frame_hdr, sizeof(frame_hdr)) != sizeof(frame_hdr)) {
        fprintf(stderr, "ERROR: Failed to write frame header: %s\n", strerror(errno));
        return -1;
    }

    /* Write frame data */
    ssize_t written = write(ctx->recording.fd, data, size);
    if (written != (ssize_t)size) {
        fprintf(stderr, "ERROR: Failed to write frame data: %s\n", strerror(errno));
        return -1;
    }

    ctx->recording.frame_count++;
    ctx->recording.bytes_written += size + sizeof(frame_hdr);

    return 0;
}

/*
 * Stop recording and close file
 */
void recording_cleanup(rootstream_ctx_t *ctx) {
    if (!ctx || !ctx->recording.active) {
        return;
    }

    if (ctx->recording.fd >= 0) {
        /* Flush and close */
        fsync(ctx->recording.fd);
        close(ctx->recording.fd);
        ctx->recording.fd = -1;
    }

    uint64_t duration_ms = (get_timestamp_us() - ctx->recording.start_time_us) / 1000;
    double duration_sec = duration_ms / 1000.0;
    double size_mb = ctx->recording.bytes_written / (1024.0 * 1024.0);
    double avg_bitrate_mbps = (ctx->recording.bytes_written * 8.0) / duration_sec / 1000000.0;

    printf("✓ Recording stopped: %s\n", ctx->recording.filename);
    printf("  Duration: %.1f seconds\n", duration_sec);
    printf("  Frames: %lu\n", ctx->recording.frame_count);
    printf("  Size: %.1f MB\n", size_mb);
    printf("  Average bitrate: %.1f Mbps\n", avg_bitrate_mbps);

    ctx->recording.active = false;
    ctx->recording.frame_count = 0;
    ctx->recording.bytes_written = 0;
}

/*
 * Read RSTR file header
 */
int rstr_read_header(int fd, rstr_header_t *header) {
    if (fd < 0 || !header) {
        return -1;
    }

    if (read(fd, header, sizeof(rstr_header_t)) != sizeof(rstr_header_t)) {
        fprintf(stderr, "ERROR: Failed to read RSTR header\n");
        return -1;
    }

    if (header->magic != RSTR_MAGIC) {
        fprintf(stderr, "ERROR: Invalid RSTR file (bad magic: 0x%08X)\n", header->magic);
        return -1;
    }

    if (header->version != RSTR_VERSION) {
        fprintf(stderr, "ERROR: Unsupported RSTR version: %u\n", header->version);
        return -1;
    }

    return 0;
}

/*
 * Read next frame from RSTR file
 * Returns 0 on success, -1 on error, 1 on EOF
 */
int rstr_read_frame(int fd, uint8_t *buffer, size_t buffer_size,
                    rstr_frame_header_t *frame_hdr) {
    if (fd < 0 || !buffer || !frame_hdr) {
        return -1;
    }

    /* Read frame header */
    ssize_t read_bytes = read(fd, frame_hdr, sizeof(rstr_frame_header_t));
    if (read_bytes == 0) {
        return 1;  /* EOF */
    }
    if (read_bytes != sizeof(rstr_frame_header_t)) {
        fprintf(stderr, "ERROR: Failed to read frame header\n");
        return -1;
    }

    if (frame_hdr->size > buffer_size) {
        fprintf(stderr, "ERROR: Frame size %u exceeds buffer size %zu\n",
                frame_hdr->size, buffer_size);
        return -1;
    }

    /* Read frame data */
    if (read(fd, buffer, frame_hdr->size) != (ssize_t)frame_hdr->size) {
        fprintf(stderr, "ERROR: Failed to read frame data\n");
        return -1;
    }

    return 0;
}
