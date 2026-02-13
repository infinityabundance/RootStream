/**
 * @file color_space.c
 * @brief Color space conversion utilities implementation
 */

#include "color_space.h"
#include <string.h>

/**
 * BT.709 YUV to RGB conversion matrix (row-major)
 * 
 * Conversion formula for limited range (16-235):
 * R = 1.164(Y - 16) + 1.596(V - 128)
 * G = 1.164(Y - 16) - 0.391(U - 128) - 0.813(V - 128)
 * B = 1.164(Y - 16) + 2.018(U - 128)
 * 
 * Matrix form:
 * | R |   | 1.164  0.000  1.596 |   | Y - 16  |
 * | G | = | 1.164 -0.391 -0.813 | * | U - 128 |
 * | B |   | 1.164  2.018  0.000 |   | V - 128 |
 */
const float YUV_TO_RGB_MATRIX[9] = {
    1.164f,  1.164f,  1.164f,  // Column 0 (Y contribution)
    0.000f, -0.391f,  2.018f,  // Column 1 (U contribution)
    1.596f, -0.813f,  0.000f   // Column 2 (V contribution)
};

/**
 * YUV offset values for limited range video
 * Y:  16/255 = 0.0625
 * UV: 128/255 = 0.5
 */
const float YUV_OFFSETS[3] = {
    16.0f / 255.0f,   // Y offset
    128.0f / 255.0f,  // U offset
    128.0f / 255.0f   // V offset
};

void color_space_get_yuv_to_rgb_matrix(float matrix[9]) {
    memcpy(matrix, YUV_TO_RGB_MATRIX, sizeof(YUV_TO_RGB_MATRIX));
}

void color_space_get_yuv_offsets(float offsets[3]) {
    memcpy(offsets, YUV_OFFSETS, sizeof(YUV_OFFSETS));
}
