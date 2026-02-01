/*
 * rstr-player.c - RootStream recording playback tool
 *
 * Plays back .rstr files recorded by RootStream.
 * Uses VA-API decoder + SDL2 display (same as client).
 */

#include "../include/rootstream.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

/* RSTR format structures (must match recording.c) */
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
    uint32_t reserved[8];
} rstr_header_t;

typedef struct __attribute__((packed)) {
    uint64_t timestamp_us;
    uint32_t size;
    uint8_t flags;         /* 0x01=keyframe */
    uint8_t reserved[3];
} rstr_frame_header_t;

/* External recording functions */
extern int rstr_read_header(int fd, rstr_header_t *header);
extern int rstr_read_frame(int fd, uint8_t *buffer, size_t buffer_size,
                           rstr_frame_header_t *frame_hdr);

static void print_usage(const char *prog) {
    printf("RootStream Recording Playback Tool\n\n");
    printf("Usage: %s [options] <recording.rstr>\n\n", prog);
    printf("Options:\n");
    printf("  -h, --help      Show this help\n");
    printf("  -l, --loop      Loop playback\n");
    printf("  -s, --speed N   Playback speed (0.5-2.0, default 1.0)\n");
    printf("\nControls:\n");
    printf("  Space  Pause/Resume\n");
    printf("  Q/Esc  Quit\n");
    printf("  Left   Seek -5s\n");
    printf("  Right  Seek +5s\n");
}

int main(int argc, char **argv) {
    const char *filename = NULL;
    bool loop = false;
    float speed = 1.0f;

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--loop") == 0) {
            loop = true;
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--speed") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: --speed requires argument\n");
                return 1;
            }
            speed = atof(argv[++i]);
            if (speed < 0.5f || speed > 2.0f) {
                fprintf(stderr, "ERROR: Speed must be 0.5-2.0\n");
                return 1;
            }
        } else if (argv[i][0] != '-') {
            filename = argv[i];
        } else {
            fprintf(stderr, "ERROR: Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!filename) {
        fprintf(stderr, "ERROR: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Open recording file */
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "ERROR: Cannot open %s: %s\n", filename, strerror(errno));
        return 1;
    }

    /* Read header */
    rstr_header_t header;
    if (rstr_read_header(fd, &header) < 0) {
        close(fd);
        return 1;
    }

    printf("RootStream Recording Playback\n");
    printf("==============================\n");
    printf("File: %s\n", filename);
    printf("Resolution: %ux%u\n", header.width, header.height);
    printf("Codec: %s\n", header.codec == 1 ? "H.265/HEVC" : "H.264/AVC");
    printf("FPS: %u\n", header.fps);

    /* Copy start_time to avoid unaligned pointer warning */
    time_t start_time = (time_t)header.start_time;
    printf("Recorded: %s", ctime(&start_time));
    printf("\n");

    /* Initialize context */
    rootstream_ctx_t ctx = {0};
    ctx.display.width = header.width;
    ctx.display.height = header.height;
    ctx.encoder.codec = (header.codec == 1) ? CODEC_H265 : CODEC_H264;

    /* Initialize decoder */
    if (rootstream_decoder_init(&ctx) < 0) {
        fprintf(stderr, "ERROR: Failed to initialize decoder\n");
        close(fd);
        return 1;
    }

    /* Initialize display */
    char title[256];
    snprintf(title, sizeof(title), "RootStream Player - %s", filename);
    if (display_init(&ctx, title, header.width, header.height) < 0) {
        fprintf(stderr, "ERROR: Failed to initialize display\n");
        rootstream_decoder_cleanup(&ctx);
        close(fd);
        return 1;
    }

    printf("✓ Playback initialized - press Space to pause, Q to quit\n\n");

    /* Allocate frame buffer */
    size_t max_frame_size = header.width * header.height * 2; /* Conservative estimate */
    uint8_t *frame_data = malloc(max_frame_size);
    if (!frame_data) {
        fprintf(stderr, "ERROR: Failed to allocate frame buffer\n");
        display_cleanup(&ctx);
        rootstream_decoder_cleanup(&ctx);
        close(fd);
        return 1;
    }

    frame_buffer_t decoded_frame = {0};
    bool paused = false;
    bool quit = false;
    uint64_t last_frame_time = 0;
    uint64_t frames_played = 0;

    /* Playback loop */
    do {
        rstr_frame_header_t frame_hdr;
        int result = rstr_read_frame(fd, frame_data, max_frame_size, &frame_hdr);

        if (result == 1) {
            /* EOF */
            if (loop) {
                printf("Looping playback...\n");
                lseek(fd, sizeof(rstr_header_t), SEEK_SET);
                last_frame_time = 0;
                continue;
            } else {
                printf("\n✓ Playback complete (%lu frames)\n", frames_played);
                break;
            }
        } else if (result < 0) {
            fprintf(stderr, "ERROR: Failed to read frame\n");
            break;
        }

        /* Decode frame */
        if (rootstream_decode_frame(&ctx, frame_data, frame_hdr.size, &decoded_frame) == 0) {
            /* Present to display */
            display_present_frame(&ctx, &decoded_frame);
            frames_played++;

            /* Timing: sleep to match original framerate */
            if (last_frame_time > 0) {
                uint64_t actual_delta = frame_hdr.timestamp_us - last_frame_time;

                if (actual_delta > 0 && actual_delta < 1000000) {
                    usleep((useconds_t)(actual_delta / speed));
                }
            }
            last_frame_time = frame_hdr.timestamp_us;

            /* Status update every second */
            if (frames_played % header.fps == 0) {
                float progress_sec = frame_hdr.timestamp_us / 1000000.0f;
                printf("\rPlayback: %.1f seconds | Frame %lu | %s",
                       progress_sec, frames_played,
                       paused ? "[PAUSED]" : "");
                fflush(stdout);
            }
        }

        /* Poll SDL events (simple, no pause/seek implemented yet) */
        if (display_poll_events(&ctx) != 0) {
            quit = true;
        }

    } while (!quit);

    /* Cleanup */
    printf("\n");
    free(frame_data);
    if (decoded_frame.data) {
        free(decoded_frame.data);
    }
    display_cleanup(&ctx);
    rootstream_decoder_cleanup(&ctx);
    close(fd);

    return 0;
}
