#ifndef RECORDING_METADATA_H
#define RECORDING_METADATA_H

#include "recording_types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize recording metadata
 * 
 * @param metadata  Metadata structure to initialize
 * @return          0 on success, -1 on error
 */
int recording_metadata_init(recording_metadata_t *metadata);

/**
 * Add a chapter marker to the recording
 * 
 * @param metadata    Metadata structure
 * @param timestamp_us Timestamp in microseconds
 * @param title       Chapter title
 * @param description Optional description (can be NULL)
 * @return            0 on success, -1 on error
 */
int recording_metadata_add_chapter(recording_metadata_t *metadata,
                                   uint64_t timestamp_us,
                                   const char *title,
                                   const char *description);

/**
 * Add an audio track to the recording
 * 
 * @param metadata    Metadata structure
 * @param name        Track name (e.g., "Game Audio", "Microphone")
 * @param channels    Number of channels
 * @param sample_rate Sample rate in Hz
 * @param enabled     Whether track is enabled
 * @return            Track ID on success, -1 on error
 */
int recording_metadata_add_audio_track(recording_metadata_t *metadata,
                                       const char *name,
                                       uint8_t channels,
                                       uint32_t sample_rate,
                                       bool enabled);

/**
 * Set game information in metadata
 * 
 * @param metadata     Metadata structure
 * @param game_name    Name of the game
 * @param game_version Version of the game (can be NULL)
 * @return             0 on success, -1 on error
 */
int recording_metadata_set_game_info(recording_metadata_t *metadata,
                                     const char *game_name,
                                     const char *game_version);

/**
 * Set player information in metadata
 * 
 * @param metadata    Metadata structure
 * @param player_name Name of the player
 * @return            0 on success, -1 on error
 */
int recording_metadata_set_player_info(recording_metadata_t *metadata,
                                       const char *player_name);

/**
 * Add tags to the recording
 * 
 * @param metadata  Metadata structure
 * @param tags      Comma-separated list of tags
 * @return          0 on success, -1 on error
 */
int recording_metadata_add_tags(recording_metadata_t *metadata,
                                const char *tags);

/**
 * Write metadata to MP4 file
 * 
 * @param metadata  Metadata structure
 * @param filename  MP4 file to write metadata to
 * @return          0 on success, -1 on error
 */
int recording_metadata_write_to_mp4(const recording_metadata_t *metadata,
                                    const char *filename);

/**
 * Write metadata to Matroska file
 * 
 * @param metadata  Metadata structure
 * @param filename  MKV file to write metadata to
 * @return          0 on success, -1 on error
 */
int recording_metadata_write_to_mkv(const recording_metadata_t *metadata,
                                    const char *filename);

/**
 * Get chapter marker by index
 * 
 * @param metadata  Metadata structure
 * @param index     Chapter index
 * @return          Pointer to chapter marker, or NULL if invalid index
 */
const chapter_marker_t* recording_metadata_get_chapter(const recording_metadata_t *metadata,
                                                       uint32_t index);

/**
 * Get audio track by ID
 * 
 * @param metadata  Metadata structure
 * @param track_id  Track ID
 * @return          Pointer to track info, or NULL if invalid ID
 */
const audio_track_info_t* recording_metadata_get_track(const recording_metadata_t *metadata,
                                                       uint32_t track_id);

#ifdef __cplusplus
}
#endif

#endif /* RECORDING_METADATA_H */
