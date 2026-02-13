# Test Fixtures for Video Renderer

This directory contains test fixtures for the video renderer implementation.

## NV12 Test Frames

The `generate_test_frames.py` script generates NV12-format test frames at various resolutions:

- **1080p**: 1920x1080
- **720p**: 1280x720
- **480p**: 640x480

### Patterns

Each resolution includes frames with different test patterns:

- **gray**: Uniform gray (Y=128, U=128, V=128)
- **black**: Black (Y=16, U=128, V=128)
- **white**: White (Y=235, U=128, V=128)
- **red**: Red (Y=82, U=90, V=240)
- **green**: Green (Y=145, U=54, V=34)
- **blue**: Blue (Y=41, U=240, V=110)
- **gradient**: Horizontal luminance gradient

### Usage

Generate test frames:

```bash
./generate_test_frames.py
```

This will create `.raw` files containing NV12 frame data.

### File Format

Files are raw NV12 data:
- Y plane: `width * height` bytes
- UV plane: `(width/2) * (height/2) * 2` bytes (interleaved U and V)

Total size = `width * height * 3 / 2` bytes

### Loading in Tests

```c
// Example: Load 720p gray frame
FILE *f = fopen("nv12_720p_gray.raw", "rb");
frame_t frame;
frame.width = 1280;
frame.height = 720;
frame.size = 1280 * 720 * 3 / 2;
frame.format = 0x3231564E; // NV12 fourcc
frame.data = malloc(frame.size);
fread(frame.data, 1, frame.size, f);
fclose(f);
```

## Reference RGB Outputs

Expected RGB output values for color space conversion validation:

| Input (YUV) | Expected Output (RGB) |
|-------------|-----------------------|
| Black (16, 128, 128) | (0, 0, 0) |
| White (235, 128, 128) | (255, 255, 255) |
| Red (82, 90, 240) | (255, 0, 0) ±5 |
| Green (145, 54, 34) | (0, 255, 0) ±5 |
| Blue (41, 240, 110) | (0, 0, 255) ±5 |
| Gray (128, 128, 128) | (128, 128, 128) ±5 |

*Note: ±5 tolerance accounts for rounding in color space conversion*
