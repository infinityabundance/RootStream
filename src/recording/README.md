# Phase 18: Stream Recording System

## Overview

The recording system provides comprehensive functionality to capture and save gameplay streams to disk with multiple codec and quality options.

## Features

### Video Codecs
- **H.264** - Fast encoding, universal compatibility (primary)
- **VP9** - Better compression, open-source (future)
- **AV1** - Best compression, modern codec (future)

### Audio Codecs
- **Opus** - Passthrough from stream (no re-encoding)
- **AAC** - Compatible fallback (future)

### Container Formats
- **MP4** - Universal compatibility (requires FFmpeg)
- **Matroska (MKV)** - Advanced features (future)

### Quality Presets
- **Fast** - H.264 veryfast, 20Mbps, AAC, MP4
- **Balanced** - H.264 medium, 8Mbps, Opus, MP4 (default)
- **High Quality** - VP9, 5Mbps, Opus, MKV (future)
- **Archival** - AV1, 2Mbps, Opus, MKV (future)

## Architecture

```
┌─────────────────────────────────────┐
│  Video/Audio Capture Pipeline       │
└──────────┬──────────────────────────┘
           │
           ├─→ Display/Playback
           │
           └─→ Recording Pipeline
               │
               ├─→ Disk Manager
               │   ├─ Space monitoring
               │   ├─ Auto-cleanup
               │   └─ File organization
               │
               └─→ Recording Manager
                   ├─ Video encoding
                   ├─ Audio encoding
                   ├─ Muxing (MP4/MKV)
                   └─ File writing
```

## Components

### Recording Types (`recording_types.h`)
Core data structures for recording system:
- `recording_info_t` - Recording metadata and status
- `video_frame_t` - Video frame data
- `audio_chunk_t` - Audio sample data
- Codec and preset enumerations

### Recording Presets (`recording_presets.h`)
Predefined quality/performance configurations:
- Codec selection
- Bitrate settings
- Container format
- Encoding parameters

### Disk Manager (`disk_manager.cpp`)
Storage and file management:
- Directory creation and management
- Disk space monitoring
- Automatic cleanup of old recordings
- Filename generation with timestamps
- Storage limit enforcement

### Recording Manager (`recording_manager.cpp`)
Main recording coordinator:
- Recording start/stop/pause/resume
- Frame queue management
- Muxer initialization
- Statistics tracking

## Build Requirements

### Required Dependencies
- C++17 compiler
- libavformat (FFmpeg)
- libavcodec (FFmpeg)
- libavutil (FFmpeg)

### Optional Dependencies
- libx264 (H.264 encoding)
- libvpx (VP9 encoding)
- libaom (AV1 encoding)
- libfdk-aac (AAC encoding)

## Building

```bash
# Install FFmpeg development libraries (Ubuntu/Debian)
sudo apt-get install libavformat-dev libavcodec-dev libavutil-dev

# Configure and build
mkdir build && cd build
cmake ..
make

# Run tests
ctest -R recording
```

## Usage

### C++ API

```cpp
#include "recording/recording_manager.h"

// Initialize
RecordingManager recorder;
recorder.init("recordings");

// Start recording
recorder.start_recording(PRESET_BALANCED, "MyGame");

// Submit frames (from your capture pipeline)
recorder.submit_video_frame(frame_data, width, height, "rgb", timestamp);
recorder.submit_audio_chunk(samples, sample_count, sample_rate, timestamp);

// Stop recording
recorder.stop_recording();

// Query status
const recording_info_t* info = recorder.get_active_recording();
uint64_t file_size = recorder.get_current_file_size();
```

### Configuration

```cpp
// Set output directory
recorder.set_output_directory("/path/to/recordings");

// Set storage limit (in MB)
recorder.set_max_storage(10000);  // 10GB

// Enable auto-cleanup
recorder.set_auto_cleanup(true, 90);  // Cleanup at 90% usage
```

## Testing

### Unit Tests

```bash
# Test recording types
./build/test_recording_types

# Test disk manager
./build/test_disk_manager
```

### Integration Tests

Integration tests require FFmpeg libraries and will test:
- Full recording pipeline
- Video/audio synchronization
- File playback verification

## File Format

Recordings are saved in standard container formats:

- **MP4** - `.mp4` files compatible with all media players
- **Matroska** - `.mkv` files for advanced features (future)

Files are named with timestamps:
- `recording_YYYYMMDD_HHMMSS.mp4`
- `GameName_YYYYMMDD_HHMMSS.mp4` (with game name)

## Performance

### CPU Usage
- H.264 (fast preset): ~5-10% single core
- H.264 (medium preset): ~10-20% single core
- VP9: ~20-40% single core (future)
- AV1: ~40-80% single core (future)

### Disk I/O
- Fast preset: ~20 Mbps sequential writes
- Balanced preset: ~8-10 Mbps sequential writes
- High quality: ~5 Mbps sequential writes

## Limitations

Current implementation:
- ✅ Basic recording framework
- ✅ Disk space management
- ✅ File organization
- ✅ MP4 container support
- ⚠️ Video encoding integration (requires encoder hookup)
- ⚠️ Audio encoding integration (requires audio hookup)
- ⚠️ Replay buffer (future)
- ⚠️ VP9/AV1 codecs (future)
- ⚠️ Recording UI (future)

## Future Enhancements

### Phase 18.1: Full Codec Support
- VP9 video encoding
- AV1 video encoding
- AAC audio encoding
- Matroska container

### Phase 18.2: Advanced Features
- Instant replay buffer
- Chapter markers
- Metadata tagging
- Multiple audio tracks

### Phase 18.3: UI Integration
- Recording dialog (Qt)
- Quality presets selector
- Live preview
- Storage management UI

## Contributing

When extending the recording system:

1. Add new codecs in `recording_types.h`
2. Update presets in `recording_presets.h`
3. Implement encoder wrapper classes
4. Add tests for new features
5. Update documentation

## License

Same as RootStream project license.
