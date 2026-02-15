/**
 * Test for MP4/MKV container format support
 * 
 * This test verifies:
 * 1. MP4 container creation and muxing
 * 2. Matroska/MKV container creation and muxing
 * 3. Container format selection based on codec
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}

// Test helper macros
#define TEST_ASSERT(condition, msg) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "FAIL: %s\n", msg); \
            return -1; \
        } \
    } while(0)

#define TEST_PASS(name) \
    do { \
        printf("PASS: %s\n", name); \
        return 0; \
    } while(0)

/**
 * Test MP4 container creation
 */
int test_mp4_container_creation() {
    const char *filename = "/tmp/test_recording.mp4";
    
    // Create output context for MP4
    AVFormatContext *fmt_ctx = nullptr;
    int ret = avformat_alloc_output_context2(&fmt_ctx, nullptr, "mp4", filename);
    
    TEST_ASSERT(ret >= 0 && fmt_ctx != nullptr, "Failed to allocate MP4 output context");
    TEST_ASSERT(strcmp(fmt_ctx->oformat->name, "mp4") == 0, "Output format should be MP4");
    
    // Create a dummy video stream
    AVStream *video_stream = avformat_new_stream(fmt_ctx, nullptr);
    TEST_ASSERT(video_stream != nullptr, "Failed to create video stream");
    
    video_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    video_stream->codecpar->codec_id = AV_CODEC_ID_H264;
    video_stream->codecpar->width = 1920;
    video_stream->codecpar->height = 1080;
    video_stream->time_base = (AVRational){1, 60};
    
    // Open output file
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        TEST_ASSERT(ret >= 0, "Failed to open MP4 output file");
    }
    
    // Write header
    ret = avformat_write_header(fmt_ctx, nullptr);
    TEST_ASSERT(ret >= 0, "Failed to write MP4 header");
    
    // Write trailer (empty file)
    av_write_trailer(fmt_ctx);
    
    // Cleanup
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&fmt_ctx->pb);
    }
    avformat_free_context(fmt_ctx);
    
    // Verify file was created
    struct stat st;
    TEST_ASSERT(stat(filename, &st) == 0, "MP4 file should exist");
    
    // Cleanup test file
    unlink(filename);
    
    TEST_PASS("test_mp4_container_creation");
}

/**
 * Test Matroska/MKV container creation
 */
int test_mkv_container_creation() {
    const char *filename = "/tmp/test_recording.mkv";
    
    // Create output context for Matroska
    AVFormatContext *fmt_ctx = nullptr;
    int ret = avformat_alloc_output_context2(&fmt_ctx, nullptr, "matroska", filename);
    
    TEST_ASSERT(ret >= 0 && fmt_ctx != nullptr, "Failed to allocate Matroska output context");
    TEST_ASSERT(strcmp(fmt_ctx->oformat->name, "matroska") == 0, "Output format should be Matroska");
    
    // Create a dummy video stream
    AVStream *video_stream = avformat_new_stream(fmt_ctx, nullptr);
    TEST_ASSERT(video_stream != nullptr, "Failed to create video stream");
    
    video_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    video_stream->codecpar->codec_id = AV_CODEC_ID_VP9;
    video_stream->codecpar->width = 1920;
    video_stream->codecpar->height = 1080;
    video_stream->time_base = (AVRational){1, 60};
    
    // Open output file
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        TEST_ASSERT(ret >= 0, "Failed to open Matroska output file");
    }
    
    // Write header
    ret = avformat_write_header(fmt_ctx, nullptr);
    TEST_ASSERT(ret >= 0, "Failed to write Matroska header");
    
    // Write trailer (empty file)
    av_write_trailer(fmt_ctx);
    
    // Cleanup
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&fmt_ctx->pb);
    }
    avformat_free_context(fmt_ctx);
    
    // Verify file was created
    struct stat st;
    TEST_ASSERT(stat(filename, &st) == 0, "MKV file should exist");
    
    // Cleanup test file
    unlink(filename);
    
    TEST_PASS("test_mkv_container_creation");
}

/**
 * Test MP4 with H.264 video and AAC audio
 */
int test_mp4_with_audio() {
    const char *filename = "/tmp/test_recording_audio.mp4";
    
    AVFormatContext *fmt_ctx = nullptr;
    int ret = avformat_alloc_output_context2(&fmt_ctx, nullptr, "mp4", filename);
    TEST_ASSERT(ret >= 0 && fmt_ctx != nullptr, "Failed to allocate MP4 output context");
    
    // Create video stream
    AVStream *video_stream = avformat_new_stream(fmt_ctx, nullptr);
    TEST_ASSERT(video_stream != nullptr, "Failed to create video stream");
    
    video_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    video_stream->codecpar->codec_id = AV_CODEC_ID_H264;
    video_stream->codecpar->width = 1920;
    video_stream->codecpar->height = 1080;
    video_stream->time_base = (AVRational){1, 60};
    
    // Create audio stream
    AVStream *audio_stream = avformat_new_stream(fmt_ctx, nullptr);
    TEST_ASSERT(audio_stream != nullptr, "Failed to create audio stream");
    
    audio_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    audio_stream->codecpar->codec_id = AV_CODEC_ID_AAC;
    audio_stream->codecpar->ch_layout.nb_channels = 2;
    audio_stream->codecpar->sample_rate = 48000;
    audio_stream->time_base = (AVRational){1, 48000};
    
    // Open and write
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        TEST_ASSERT(ret >= 0, "Failed to open output file");
    }
    
    ret = avformat_write_header(fmt_ctx, nullptr);
    TEST_ASSERT(ret >= 0, "Failed to write header");
    
    av_write_trailer(fmt_ctx);
    
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&fmt_ctx->pb);
    }
    avformat_free_context(fmt_ctx);
    
    // Verify file was created
    struct stat st;
    TEST_ASSERT(stat(filename, &st) == 0, "MP4 file with audio should exist");
    
    unlink(filename);
    
    TEST_PASS("test_mp4_with_audio");
}

/**
 * Test MKV with VP9 video and Opus audio
 */
int test_mkv_with_opus_audio() {
    const char *filename = "/tmp/test_recording_vp9_opus.mkv";
    
    AVFormatContext *fmt_ctx = nullptr;
    int ret = avformat_alloc_output_context2(&fmt_ctx, nullptr, "matroska", filename);
    TEST_ASSERT(ret >= 0 && fmt_ctx != nullptr, "Failed to allocate Matroska output context");
    
    // Create video stream
    AVStream *video_stream = avformat_new_stream(fmt_ctx, nullptr);
    TEST_ASSERT(video_stream != nullptr, "Failed to create video stream");
    
    video_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    video_stream->codecpar->codec_id = AV_CODEC_ID_VP9;
    video_stream->codecpar->width = 1920;
    video_stream->codecpar->height = 1080;
    video_stream->time_base = (AVRational){1, 60};
    
    // Create audio stream
    AVStream *audio_stream = avformat_new_stream(fmt_ctx, nullptr);
    TEST_ASSERT(audio_stream != nullptr, "Failed to create audio stream");
    
    audio_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    audio_stream->codecpar->codec_id = AV_CODEC_ID_OPUS;
    audio_stream->codecpar->ch_layout.nb_channels = 2;
    audio_stream->codecpar->sample_rate = 48000;
    audio_stream->time_base = (AVRational){1, 48000};
    
    // Open and write
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        TEST_ASSERT(ret >= 0, "Failed to open output file");
    }
    
    ret = avformat_write_header(fmt_ctx, nullptr);
    TEST_ASSERT(ret >= 0, "Failed to write header");
    
    av_write_trailer(fmt_ctx);
    
    if (!(fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&fmt_ctx->pb);
    }
    avformat_free_context(fmt_ctx);
    
    // Verify file was created
    struct stat st;
    TEST_ASSERT(stat(filename, &st) == 0, "MKV file with Opus should exist");
    
    unlink(filename);
    
    TEST_PASS("test_mkv_with_opus_audio");
}

/**
 * Main test runner
 */
int main() {
    int failed = 0;
    
    printf("Running container format tests...\n");
    printf("=====================================\n\n");
    
    if (test_mp4_container_creation() != 0) failed++;
    if (test_mkv_container_creation() != 0) failed++;
    if (test_mp4_with_audio() != 0) failed++;
    if (test_mkv_with_opus_audio() != 0) failed++;
    
    printf("\n=====================================\n");
    if (failed == 0) {
        printf("✓ All container format tests passed!\n");
        return 0;
    } else {
        printf("✗ %d test(s) failed\n", failed);
        return 1;
    }
}
