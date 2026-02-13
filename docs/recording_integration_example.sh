#!/bin/bash
# Example integration of recording system with RootStream
# This demonstrates how recording would be called from the main streaming loop

cat << 'EOF'
╔══════════════════════════════════════════════════════════════╗
║ RootStream Recording System Integration Example             ║
╚══════════════════════════════════════════════════════════════╝

This example shows how to integrate the recording system with
RootStream's main streaming pipeline.

┌──────────────────────────────────────────────────────────────┐
│ 1. C++ API Usage                                             │
└──────────────────────────────────────────────────────────────┘

#include "recording/recording_manager.h"

// Initialize recording system
RecordingManager recorder;
recorder.init("recordings");
recorder.set_max_storage(10000);  // 10GB limit
recorder.set_auto_cleanup(true, 90);  // Cleanup at 90% full

// Start recording with preset
recorder.start_recording(PRESET_BALANCED, "MyGame");

// In your streaming loop:
while (streaming) {
    // Capture frame
    uint8_t *frame = capture_video_frame();
    float *audio = capture_audio_samples();
    
    // Submit to both streaming and recording
    stream_send_frame(frame);  // Send over network
    
    if (recorder.is_recording_active()) {
        recorder.submit_video_frame(
            frame, width, height, "rgb", timestamp_us);
        recorder.submit_audio_chunk(
            audio, sample_count, sample_rate, timestamp_us);
    }
}

// Stop recording
recorder.stop_recording();


┌──────────────────────────────────────────────────────────────┐
│ 2. Quality Presets                                           │
└──────────────────────────────────────────────────────────────┘

PRESET_FAST (20Mbps H.264):
  - Use case: Quick captures, minimal CPU impact
  - Target: ~5-10% CPU overhead
  - File size: ~1.5GB per 10 minutes (1080p)

PRESET_BALANCED (8Mbps H.264) - DEFAULT:
  - Use case: Daily recording, good balance
  - Target: ~10-15% CPU overhead
  - File size: ~600MB per 10 minutes (1080p)

PRESET_HIGH_QUALITY (5Mbps VP9):
  - Use case: High quality archives
  - Target: ~20-30% CPU overhead
  - File size: ~375MB per 10 minutes (1080p)

PRESET_ARCHIVAL (2Mbps AV1):
  - Use case: Long-term storage
  - Target: ~40-60% CPU overhead
  - File size: ~150MB per 10 minutes (1080p)


┌──────────────────────────────────────────────────────────────┐
│ 3. Storage Management                                        │
└──────────────────────────────────────────────────────────────┘

Auto-cleanup example:
  - Max storage: 10GB
  - Current usage: 9.5GB (95%)
  - Threshold: 90%
  - Action: Remove oldest recordings until below 80%
  
Monitoring:
  uint64_t free_space = recorder.get_available_disk_space();
  uint64_t file_size = recorder.get_current_file_size();
  uint32_t queue_depth = recorder.get_encoding_queue_depth();


┌──────────────────────────────────────────────────────────────┐
│ 4. Integration Points in RootStream                          │
└──────────────────────────────────────────────────────────────┘

main.c / service.c:
  - Parse --record flag
  - Initialize RecordingManager
  - Pass frames to recorder

encoder callbacks:
  - Hook into VAAPI/NVENC encoded output
  - Submit encoded frames to recorder

audio callbacks:
  - Hook into Opus encoder output
  - Submit audio chunks to recorder


┌──────────────────────────────────────────────────────────────┐
│ 5. File Output                                               │
└──────────────────────────────────────────────────────────────┘

Output directory: recordings/
  recording_20260213_075000.mp4
  MyGame_20260213_080530.mp4
  MyGame_20260213_091245.mp4

Playback with standard tools:
  vlc recordings/recording_20260213_075000.mp4
  ffplay recordings/MyGame_20260213_080530.mp4
  mpv recordings/MyGame_20260213_091245.mp4


┌──────────────────────────────────────────────────────────────┐
│ 6. Error Handling                                            │
└──────────────────────────────────────────────────────────────┘

if (recorder.start_recording(preset, game_name) != 0) {
    if (recorder.is_space_low()) {
        fprintf(stderr, "Low disk space!\n");
        if (auto_cleanup_enabled) {
            recorder.disk_manager->auto_cleanup_old_recordings();
        }
    } else {
        fprintf(stderr, "Failed to start recording\n");
    }
}

Monitor for frame drops:
  uint32_t drops = recorder.get_frame_drop_count();
  if (drops > 0) {
      fprintf(stderr, "Warning: %u frames dropped\n", drops);
  }


╔══════════════════════════════════════════════════════════════╗
║ Implementation Status                                        ║
╚══════════════════════════════════════════════════════════════╝

✅ Foundation complete:
   - Type definitions
   - Disk management
   - Recording manager framework
   - Build system integration
   - Unit tests

⚠️  Requires integration:
   - Encoder hookup (VAAPI/NVENC output → recorder)
   - Audio hookup (Opus encoder → recorder)
   - Main loop integration (main.c/service.c)
   - Command-line flags (--record, --preset)

🔮 Future enhancements:
   - VP9/AV1 codec support
   - Replay buffer (save last N seconds)
   - Qt UI for recording controls
   - Chapter markers and metadata

EOF
