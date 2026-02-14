# Recording System - Phase 25.1 Implementation

This directory contains the RootStream recording system implementation with support for VP9, AV1, replay buffers, chapter markers, and Qt UI controls.

## Features

### Video Encoders

#### H.264 Encoder (`h264_encoder_wrapper.h/cpp`)
- Fast, universal compatibility
- libx264 integration via FFmpeg
- Preset support (ultrafast, veryfast, fast, medium, slow, veryslow)
- CRF and bitrate modes
- Dynamic bitrate adjustment
- Keyframe forcing

#### VP9 Encoder (`vp9_encoder_wrapper.h/cpp`)
- Better compression than H.264
- libvpx-vp9 integration via FFmpeg
- CPU usage parameter (0-5, higher = faster)
- Row-based multithreading
- Tile-based parallel encoding
- Quality and bitrate modes

#### AV1 Encoder (`av1_encoder_wrapper.h/cpp`)
- Best compression ratio
- libaom-av1 integration via FFmpeg
- CPU usage parameter (0-8, higher = faster)
- Row-based multithreading
- Tile-based parallel encoding
- CRF and bitrate modes
- Note: Slower encoding than H.264/VP9

### Replay Buffer (`replay_buffer.h/cpp`)
- Circular buffer for last N seconds of gameplay
- Time-based frame eviction
- Memory limit enforcement
- Separate video and audio queues
- Save to file on demand
- Statistics tracking

### Chapter Markers & Metadata (`recording_metadata.h/cpp`)
- Chapter markers with timestamps, titles, and descriptions
- Multi-track audio metadata
- Game information (name, version)
- Player information
- Custom tags
- Session ID tracking
- MP4 and Matroska metadata writing

### Qt UI Components

#### Recording Control Widget (`recording_control_widget.h/cpp`)
- Start/Stop/Pause/Resume controls
- Preset selector (Fast/Balanced/Quality/Archival)
- Real-time status display
  - Duration counter
  - File size
  - Current bitrate
  - Queue depth
  - Frame drops
- Replay buffer save button
- Chapter marker addition
- Progress bar for queue status

#### Preview Widget (`recording_preview_widget.h/cpp`)
- Live preview of recording
- Configurable quality (0.25x - 1.0x scale)
- Throttled updates (max 30 FPS)
- Enable/disable toggle
- Minimal CPU overhead

#### Advanced Encoding Dialog (`advanced_encoding_dialog.h/cpp`)
- Tabbed interface for all encoding options
- Video settings
  - Codec selection (H.264/VP9/AV1)
  - Resolution and frame rate
  - Bitrate or CRF mode
  - Codec-specific parameters
  - GOP size and B-frames
  - Two-pass encoding option
- Audio settings
  - Codec selection (Opus/AAC)
  - Bitrate, sample rate, channels
- Container format (MP4/MKV)
- HDR support (planned for future)
- Preset loading and saving

## Recording Presets

### Fast
- Codec: H.264 (veryfast preset)
- Bitrate: 20 Mbps
- Audio: AAC 192 kbps
- Container: MP4
- Use case: Low CPU overhead, quick encoding, livestreaming

### Balanced (Default)
- Codec: H.264 (medium preset)
- Bitrate: 8-10 Mbps
- Audio: Opus 128 kbps
- Container: MP4
- Use case: Good quality/size balance for general recording

### High Quality
- Codec: VP9 (cpu-used=2)
- Bitrate: 5-8 Mbps
- Audio: Opus 128 kbps
- Container: Matroska (MKV)
- Use case: Better compression with longer encoding time

### Archival
- Codec: AV1 (cpu-used=4, CRF=30)
- Bitrate: 2-4 Mbps
- Audio: Opus 96 kbps
- Container: Matroska (MKV)
- Use case: Best compression for archival storage (very slow)

## Usage Examples

### Basic Recording with Preset
```cpp
RecordingManager manager;
manager.init("/path/to/recordings");
manager.start_recording(PRESET_BALANCED, "My Game");

// Submit frames...
manager.submit_video_frame(frame_data, width, height, "rgb", timestamp);
manager.submit_audio_chunk(audio_samples, sample_count, sample_rate, timestamp);

manager.stop_recording();
```

### Replay Buffer
```cpp
// Enable 30-second replay buffer with 500 MB max memory
manager.enable_replay_buffer(30, 500);

// Later, save replay to file
manager.save_replay_buffer("epic_moment.mp4", 30);
```

### Chapter Markers
```cpp
// Add chapter at current timestamp
manager.add_chapter_marker("Boss Fight", "First encounter");
manager.add_chapter_marker("Victory!", "Boss defeated");
```

### Advanced Encoding Options
```cpp
// Create custom encoding options
EncodingOptions options;
options.codec = VIDEO_CODEC_VP9;
options.bitrate_kbps = 6000;
options.vp9_cpu_used = 1;  // Slower but better quality
options.use_two_pass = true;

// Apply to recording...
```

## Integration with KDE Client

The Qt recording controls can be integrated into the KDE Plasma client:

```cpp
// In main window
RecordingControlWidget *recordingControls = new RecordingControlWidget();
connect(recordingControls, &RecordingControlWidget::startRecordingRequested,
        this, &MainWindow::onStartRecording);
connect(recordingControls, &RecordingControlWidget::stopRecordingRequested,
        this, &MainWindow::onStopRecording);

// Add to layout
layout->addWidget(recordingControls);
```

## Performance Considerations

### H.264 (Fast Preset)
- CPU Usage: ~5-10% single core
- Encoding Speed: Real-time at 1080p60
- File Size: ~20 MB/minute
- Latency: <10ms

### H.264 (Balanced Preset)
- CPU Usage: ~10-20% single core
- Encoding Speed: Real-time at 1080p60
- File Size: ~8-10 MB/minute
- Latency: <20ms

### VP9 (High Quality)
- CPU Usage: ~30-50% single core
- Encoding Speed: Real-time at 1080p30, may struggle at 1080p60
- File Size: ~5-8 MB/minute
- Latency: ~50ms

### AV1 (Archival)
- CPU Usage: ~50-80% single core
- Encoding Speed: NOT real-time (expect 0.5-2x speed)
- File Size: ~2-4 MB/minute
- Latency: ~200ms+
- **Note:** Best used for post-processing, not live recording

## Dependencies

- FFmpeg (libavcodec, libavformat, libavutil, libswscale)
- libx264 (for H.264)
- libvpx-vp9 (for VP9)
- libaom (for AV1)
- Qt6 (Core, Gui, Widgets) - for UI components

## Future Enhancements

- [ ] GPU-accelerated encoding (VA-API, NVENC)
- [ ] Multiple audio track recording (game + mic)
- [ ] HDR video support (HDR10, HLG)
- [ ] Stream to file while streaming to network
- [ ] Recording pause with seamless resume
- [ ] Automatic scene detection for chapters
- [ ] Recording profiles with custom settings
- [ ] Instant replay hotkey support

## Testing

To test the encoders:

```bash
# Build the test
cd tests
./test_recording_compile.sh

# Test H.264 encoding
./test_h264_encoder

# Test VP9 encoding  
./test_vp9_encoder

# Test AV1 encoding
./test_av1_encoder

# Test replay buffer
./test_replay_buffer
```

## License

This code is part of the RootStream project and follows the same MIT license.
