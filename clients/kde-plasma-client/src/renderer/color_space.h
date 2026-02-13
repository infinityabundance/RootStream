/**
 * @file color_space.h
 * @brief Color space conversion utilities for video rendering
 * 
 * Provides conversion matrices and utilities for NV12/YUV to RGB conversion
 * using BT.709 color space standard.
 */

#ifndef COLOR_SPACE_H
#define COLOR_SPACE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * BT.709 YUV to RGB conversion matrix
 * 
 * RGB = YUV_TO_RGB_MATRIX * (YUV - OFFSETS)
 * 
 * Standard formula:
 * R = 1.164(Y - 16) + 1.596(V - 128)
 * G = 1.164(Y - 16) - 0.391(U - 128) - 0.813(V - 128)
 * B = 1.164(Y - 16) + 2.018(U - 128)
 */
extern const float YUV_TO_RGB_MATRIX[9];

/**
 * YUV offset values for limited range video (16-235)
 */
extern const float YUV_OFFSETS[3];

/**
 * Get YUV to RGB conversion matrix for shader
 * 
 * @param matrix Output 3x3 matrix (row-major)
 */
void color_space_get_yuv_to_rgb_matrix(float matrix[9]);

/**
 * Get YUV offset values for shader
 * 
 * @param offsets Output offset values (Y, U, V)
 */
void color_space_get_yuv_offsets(float offsets[3]);

#ifdef __cplusplus
}
#endif

#endif /* COLOR_SPACE_H */
