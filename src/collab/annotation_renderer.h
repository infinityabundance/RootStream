/*
 * annotation_renderer.h — Render annotations onto video frames
 *
 * Maintains an in-memory stroke/text annotation layer and composites it
 * onto raw RGBA frames.  The renderer is decoupled from the display
 * backend; callers supply a pixel buffer.
 *
 * Coordinate system: normalised [0,1] × [0,1] matching the frame.
 */

#ifndef ROOTSTREAM_ANNOTATION_RENDERER_H
#define ROOTSTREAM_ANNOTATION_RENDERER_H

#include <stddef.h>
#include <stdint.h>

#include "annotation_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum strokes retained in the annotation layer */
#define ANNOT_RENDERER_MAX_STROKES 256
/** Maximum points per stroke */
#define ANNOT_RENDERER_MAX_POINTS 1024
/** Maximum text annotations */
#define ANNOT_RENDERER_MAX_TEXTS 64

/** Opaque renderer handle */
typedef struct annotation_renderer_s annotation_renderer_t;

/**
 * annotation_renderer_create — allocate renderer state
 *
 * @return Non-NULL handle, or NULL on OOM
 */
annotation_renderer_t *annotation_renderer_create(void);

/**
 * annotation_renderer_destroy — free all renderer state
 *
 * @param renderer  Renderer to destroy
 */
void annotation_renderer_destroy(annotation_renderer_t *renderer);

/**
 * annotation_renderer_apply_event — update annotation state from an event
 *
 * @param renderer  Annotation renderer
 * @param event     Decoded annotation event
 */
void annotation_renderer_apply_event(annotation_renderer_t *renderer,
                                     const annotation_event_t *event);

/**
 * annotation_renderer_composite — draw all annotations onto @pixels
 *
 * The pixel buffer must be RGBA 8bpc (4 bytes/pixel), row-major.
 * Pixels are alpha-composited using Porter-Duff src-over.
 *
 * @param renderer  Annotation renderer
 * @param pixels    RGBA frame buffer (modified in-place)
 * @param width     Frame width in pixels
 * @param height    Frame height in pixels
 * @param stride    Row stride in bytes (≥ width × 4)
 */
void annotation_renderer_composite(annotation_renderer_t *renderer, uint8_t *pixels, int width,
                                   int height, int stride);

/**
 * annotation_renderer_clear — remove all strokes and texts
 *
 * @param renderer  Annotation renderer
 */
void annotation_renderer_clear(annotation_renderer_t *renderer);

/**
 * annotation_renderer_stroke_count — return number of active strokes
 *
 * @param renderer  Annotation renderer
 * @return          Stroke count
 */
size_t annotation_renderer_stroke_count(const annotation_renderer_t *renderer);

#ifdef __cplusplus
}
#endif

#endif /* ROOTSTREAM_ANNOTATION_RENDERER_H */
