#!/usr/bin/env python3
"""
Generate test NV12 frames for renderer testing
"""

import struct
import sys

def generate_nv12_frame(width, height, pattern='gray'):
    """
    Generate a simple NV12 frame
    
    NV12 format:
    - Y plane: width x height bytes
    - UV plane: (width/2) x (height/2) x 2 bytes (interleaved U and V)
    """
    # Y plane
    y_size = width * height
    
    # UV plane
    uv_size = (width // 2) * (height // 2) * 2
    
    # Total size
    total_size = y_size + uv_size
    
    # Create frame data
    frame = bytearray(total_size)
    
    if pattern == 'gray':
        # Gray pattern (Y=128, U=128, V=128)
        frame[0:y_size] = bytes([128] * y_size)
        frame[y_size:] = bytes([128] * uv_size)
    elif pattern == 'black':
        # Black (Y=16, U=128, V=128)
        frame[0:y_size] = bytes([16] * y_size)
        frame[y_size:] = bytes([128] * uv_size)
    elif pattern == 'white':
        # White (Y=235, U=128, V=128)
        frame[0:y_size] = bytes([235] * y_size)
        frame[y_size:] = bytes([128] * uv_size)
    elif pattern == 'red':
        # Red (Y=82, U=90, V=240)
        frame[0:y_size] = bytes([82] * y_size)
        for i in range(0, uv_size, 2):
            frame[y_size + i] = 90      # U
            frame[y_size + i + 1] = 240 # V
    elif pattern == 'green':
        # Green (Y=145, U=54, V=34)
        frame[0:y_size] = bytes([145] * y_size)
        for i in range(0, uv_size, 2):
            frame[y_size + i] = 54   # U
            frame[y_size + i + 1] = 34  # V
    elif pattern == 'blue':
        # Blue (Y=41, U=240, V=110)
        frame[0:y_size] = bytes([41] * y_size)
        for i in range(0, uv_size, 2):
            frame[y_size + i] = 240  # U
            frame[y_size + i + 1] = 110  # V
    elif pattern == 'gradient':
        # Horizontal gradient (Y varies from 16 to 235)
        for y in range(height):
            for x in range(width):
                frame[y * width + x] = 16 + int((235 - 16) * x / width)
        # UV centered
        frame[y_size:] = bytes([128] * uv_size)
    
    return bytes(frame)

def main():
    # Generate test frames
    resolutions = [
        (1920, 1080, '1080p'),
        (1280, 720, '720p'),
        (640, 480, '480p'),
    ]
    
    patterns = ['gray', 'black', 'white', 'red', 'green', 'blue', 'gradient']
    
    for width, height, name in resolutions:
        for pattern in patterns:
            filename = f'nv12_{name}_{pattern}.raw'
            frame = generate_nv12_frame(width, height, pattern)
            with open(filename, 'wb') as f:
                f.write(frame)
            print(f'Generated {filename} ({len(frame)} bytes)')

if __name__ == '__main__':
    main()
