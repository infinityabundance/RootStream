/*
 * caption_renderer.h — Caption overlay compositor
 *
 * Draws active caption events onto an RGBA frame buffer.  Caption text
 * is rendered as a semi-transparent "pill" box with a configurable
 * background colour and a fixed-width pixel font.  For portability the
 * renderer uses a built-in 5×7 ASCII bitmap font; no external font
 * library is required.
 *
 * Typical usage
 * ─────────────
 *   caption_renderer_t *r = caption_renderer_create(NULL);
 *   // per frame:
 *   caption_renderer_draw(r, pixels, width, height, stride, events, n, pts_us);
 *   caption_renderer_destroy(r);
 */

#ifndef ROOTSTREAM_CAPTION_RENDERER_H
#define ROOTSTREAM_CAPTION_RENDERER_H

#include <stddef.h>
#include <stdint.h>

#include "caption_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Caption renderer configuration */
typedef struct {
    uint32_t bg_color; /**< ARGB background colour (default 0xBB000000) */
    uint32_t fg_color; /**< ARGB foreground colour (default 0xFFFFFFFF) */
    int font_scale;    /**< Pixel scale factor for built-in font (1–4) */
    int margin_px;     /**< Horizontal margin in pixels */
} caption_renderer_config_t;

/** Opaque renderer handle */
typedef struct caption_renderer_s caption_renderer_t;

/**
 * caption_renderer_create — allocate renderer
 *
 * @param config  Configuration; NULL uses sensible defaults
 * @return        Non-NULL handle, or NULL on OOM
 */
caption_renderer_t *caption_renderer_create(const caption_renderer_config_t *config);

/**
 * caption_renderer_destroy — free renderer
 *
 * @param r  Renderer to destroy
 */
void caption_renderer_destroy(caption_renderer_t *r);

/**
 * caption_renderer_draw — composite @n caption events onto @pixels
 *
 * Iterates the provided events array and draws each active one into the
 * RGBA frame buffer.  Active check uses caption_event_is_active().
 *
 * @param r       Caption renderer
 * @param pixels  RGBA frame buffer (modified in-place)
 * @param width   Frame width in pixels
 * @param height  Frame height in pixels
 * @param stride  Row stride in bytes (>= width × 4)
 * @param events  Array of caption events to consider
 * @param n       Length of @events array
 * @param now_us  Current playback timestamp in µs
 * @return        Number of captions actually rendered (>= 0)
 */
int caption_renderer_draw(caption_renderer_t *r, uint8_t *pixels, int width, int height, int stride,
                          const caption_event_t *events, int n, uint64_t now_us);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_CAPTION_RENDERER_H */
