#include "recording_metadata.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

int recording_metadata_init(recording_metadata_t *metadata) {
    if (!metadata) {
        return -1;
    }
    
    memset(metadata, 0, sizeof(recording_metadata_t));
    
    // Generate session ID based on current time
    metadata->session_id = (uint64_t)time(nullptr);
    
    return 0;
}

int recording_metadata_add_chapter(recording_metadata_t *metadata,
                                   uint64_t timestamp_us,
                                   const char *title,
                                   const char *description) {
    if (!metadata || !title) {
        return -1;
    }
    
    if (metadata->marker_count >= MAX_CHAPTER_MARKERS) {
        fprintf(stderr, "Recording Metadata: Maximum chapter markers reached\n");
        return -1;
    }
    
    chapter_marker_t *marker = &metadata->markers[metadata->marker_count];
    marker->timestamp_us = timestamp_us;
    
    strncpy(marker->title, title, sizeof(marker->title) - 1);
    marker->title[sizeof(marker->title) - 1] = '\0';
    
    if (description) {
        strncpy(marker->description, description, sizeof(marker->description) - 1);
        marker->description[sizeof(marker->description) - 1] = '\0';
    } else {
        marker->description[0] = '\0';
    }
    
    metadata->marker_count++;
    
    printf("Recording Metadata: Added chapter '%s' at %.2f seconds\n",
           title, timestamp_us / 1000000.0);
    
    return 0;
}

int recording_metadata_add_audio_track(recording_metadata_t *metadata,
                                       const char *name,
                                       uint8_t channels,
                                       uint32_t sample_rate,
                                       bool enabled) {
    if (!metadata || !name) {
        return -1;
    }
    
    if (metadata->track_count >= MAX_AUDIO_TRACKS) {
        fprintf(stderr, "Recording Metadata: Maximum audio tracks reached\n");
        return -1;
    }
    
    uint32_t track_id = metadata->track_count;
    audio_track_info_t *track = &metadata->tracks[track_id];
    
    track->track_id = track_id;
    strncpy(track->name, name, sizeof(track->name) - 1);
    track->name[sizeof(track->name) - 1] = '\0';
    track->channels = channels;
    track->sample_rate = sample_rate;
    track->enabled = enabled;
    track->volume = 1.0f;  // Default volume
    
    metadata->track_count++;
    
    printf("Recording Metadata: Added audio track '%s' (ID: %u, %u Hz, %u channels)\n",
           name, track_id, sample_rate, channels);
    
    return track_id;
}

int recording_metadata_set_game_info(recording_metadata_t *metadata,
                                     const char *game_name,
                                     const char *game_version) {
    if (!metadata || !game_name) {
        return -1;
    }
    
    strncpy(metadata->game_name, game_name, sizeof(metadata->game_name) - 1);
    metadata->game_name[sizeof(metadata->game_name) - 1] = '\0';
    
    if (game_version) {
        strncpy(metadata->game_version, game_version, sizeof(metadata->game_version) - 1);
        metadata->game_version[sizeof(metadata->game_version) - 1] = '\0';
    }
    
    return 0;
}

int recording_metadata_set_player_info(recording_metadata_t *metadata,
                                       const char *player_name) {
    if (!metadata || !player_name) {
        return -1;
    }
    
    strncpy(metadata->player_name, player_name, sizeof(metadata->player_name) - 1);
    metadata->player_name[sizeof(metadata->player_name) - 1] = '\0';
    
    return 0;
}

int recording_metadata_add_tags(recording_metadata_t *metadata,
                                const char *tags) {
    if (!metadata || !tags) {
        return -1;
    }
    
    strncpy(metadata->tags, tags, sizeof(metadata->tags) - 1);
    metadata->tags[sizeof(metadata->tags) - 1] = '\0';
    
    return 0;
}

int recording_metadata_write_to_mp4(const recording_metadata_t *metadata,
                                    const char *filename) {
    if (!metadata || !filename) {
        return -1;
    }
    
    AVFormatContext *fmt_ctx = nullptr;
    int ret = avformat_open_input(&fmt_ctx, filename, nullptr, nullptr);
    if (ret < 0) {
        fprintf(stderr, "Recording Metadata: Failed to open MP4 file\n");
        return -1;
    }
    
    // Add general metadata
    if (metadata->game_name[0] != '\0') {
        av_dict_set(&fmt_ctx->metadata, "title", metadata->game_name, 0);
    }
    
    if (metadata->player_name[0] != '\0') {
        av_dict_set(&fmt_ctx->metadata, "artist", metadata->player_name, 0);
    }
    
    if (metadata->tags[0] != '\0') {
        av_dict_set(&fmt_ctx->metadata, "comment", metadata->tags, 0);
    }
    
    char session_id_str[32];
    snprintf(session_id_str, sizeof(session_id_str), "%llu", 
             (unsigned long long)metadata->session_id);
    av_dict_set(&fmt_ctx->metadata, "session_id", session_id_str, 0);
    
    // Add chapter markers
    // Note: Adding chapters properly requires using avformat_write_header() with chapters
    // or re-muxing the file. For now, we just add metadata tags.
    // TODO: Implement proper chapter support via re-muxing or during recording
    
    char chapter_list[1024] = "";
    for (uint32_t i = 0; i < metadata->marker_count; i++) {
        const chapter_marker_t *marker = &metadata->markers[i];
        char chapter_entry[256];
        snprintf(chapter_entry, sizeof(chapter_entry), 
                 "Chapter %u: %s (%.2fs); ", 
                 i + 1, marker->title, marker->timestamp_us / 1000000.0);
        strncat(chapter_list, chapter_entry, sizeof(chapter_list) - strlen(chapter_list) - 1);
    }
    if (chapter_list[0] != '\0') {
        av_dict_set(&fmt_ctx->metadata, "chapters", chapter_list, 0);
    }
    
    avformat_close_input(&fmt_ctx);
    
    printf("Recording Metadata: Wrote metadata to MP4 file\n");
    
    return 0;
}

int recording_metadata_write_to_mkv(const recording_metadata_t *metadata,
                                    const char *filename) {
    if (!metadata || !filename) {
        return -1;
    }
    
    // Similar to MP4 but with Matroska-specific features
    // Matroska has better support for chapters and multiple tracks
    
    AVFormatContext *fmt_ctx = nullptr;
    int ret = avformat_open_input(&fmt_ctx, filename, nullptr, nullptr);
    if (ret < 0) {
        fprintf(stderr, "Recording Metadata: Failed to open MKV file\n");
        return -1;
    }
    
    // Add metadata similar to MP4
    if (metadata->game_name[0] != '\0') {
        av_dict_set(&fmt_ctx->metadata, "title", metadata->game_name, 0);
    }
    
    if (metadata->player_name[0] != '\0') {
        av_dict_set(&fmt_ctx->metadata, "artist", metadata->player_name, 0);
    }
    
    if (metadata->tags[0] != '\0') {
        av_dict_set(&fmt_ctx->metadata, "comment", metadata->tags, 0);
    }
    
    avformat_close_input(&fmt_ctx);
    
    printf("Recording Metadata: Wrote metadata to MKV file\n");
    
    return 0;
}

const chapter_marker_t* recording_metadata_get_chapter(const recording_metadata_t *metadata,
                                                       uint32_t index) {
    if (!metadata || index >= metadata->marker_count) {
        return nullptr;
    }
    
    return &metadata->markers[index];
}

const audio_track_info_t* recording_metadata_get_track(const recording_metadata_t *metadata,
                                                       uint32_t track_id) {
    if (!metadata || track_id >= metadata->track_count) {
        return nullptr;
    }
    
    return &metadata->tracks[track_id];
}
