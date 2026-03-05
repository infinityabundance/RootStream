/*
 * annotation_renderer.c — Annotation compositor implementation
 *
 * Renders strokes as thick lines using Bresenham's circle stamp, and
 * places text labels as simple ASCII bitmaps (or skips if too complex).
 * The primary goal is correctness and portability; quality is secondary.
 */

#include "annotation_renderer.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ── Internal structures ─────────────────────────────────────────── */

typedef struct {
    annot_point_t points[ANNOT_RENDERER_MAX_POINTS];
    int           count;
    annot_color_t color;
    float         width;
    uint32_t      stroke_id;
    bool          finished;
} stroke_t;

typedef struct {
    annot_point_t pos;
    annot_color_t color;
    float         font_size;
    char          text[ANNOTATION_MAX_TEXT + 1];
    uint16_t      text_len;
} text_label_t;

struct annotation_renderer_s {
    stroke_t     strokes[ANNOT_RENDERER_MAX_STROKES];
    int          stroke_count;
    text_label_t texts[ANNOT_RENDERER_MAX_TEXTS];
    int          text_count;
};

annotation_renderer_t *annotation_renderer_create(void) {
    return calloc(1, sizeof(annotation_renderer_t));
}

void annotation_renderer_destroy(annotation_renderer_t *renderer) {
    free(renderer);
}

void annotation_renderer_clear(annotation_renderer_t *renderer) {
    if (!renderer) return;
    renderer->stroke_count = 0;
    renderer->text_count   = 0;
}

size_t annotation_renderer_stroke_count(
        const annotation_renderer_t *renderer) {
    return renderer ? (size_t)renderer->stroke_count : 0;
}

/* ── Event application ──────────────────────────────────────────── */

static stroke_t *find_stroke(annotation_renderer_t *r, uint32_t stroke_id) {
    for (int i = 0; i < r->stroke_count; i++) {
        if (r->strokes[i].stroke_id == stroke_id) return &r->strokes[i];
    }
    return NULL;
}

void annotation_renderer_apply_event(annotation_renderer_t    *renderer,
                                     const annotation_event_t *event) {
    if (!renderer || !event) return;

    switch (event->type) {
    case ANNOT_DRAW_BEGIN:
        if (renderer->stroke_count >= ANNOT_RENDERER_MAX_STROKES) return;
        {
            stroke_t *s = &renderer->strokes[renderer->stroke_count++];
            memset(s, 0, sizeof(*s));
            s->stroke_id = event->draw_begin.stroke_id;
            s->color     = event->draw_begin.color;
            s->width     = event->draw_begin.width;
            s->points[0] = event->draw_begin.pos;
            s->count     = 1;
        }
        break;

    case ANNOT_DRAW_POINT:
        {
            stroke_t *s = find_stroke(renderer,
                                      event->draw_point.stroke_id);
            if (s && s->count < ANNOT_RENDERER_MAX_POINTS && !s->finished) {
                s->points[s->count++] = event->draw_point.pos;
            }
        }
        break;

    case ANNOT_DRAW_END:
        {
            stroke_t *s = find_stroke(renderer, event->draw_end.stroke_id);
            if (s) s->finished = true;
        }
        break;

    case ANNOT_ERASE:
        {
            float cx = event->erase.center.x;
            float cy = event->erase.center.y;
            float r2 = event->erase.radius * event->erase.radius;
            /* Compact strokes whose centroid lies inside the erase circle */
            int out = 0;
            for (int i = 0; i < renderer->stroke_count; i++) {
                stroke_t *s = &renderer->strokes[i];
                bool erase = false;
                for (int p = 0; p < s->count; p++) {
                    float dx = s->points[p].x - cx;
                    float dy = s->points[p].y - cy;
                    if (dx*dx + dy*dy <= r2) { erase = true; break; }
                }
                if (!erase) renderer->strokes[out++] = *s;
            }
            renderer->stroke_count = out;
        }
        break;

    case ANNOT_CLEAR_ALL:
        annotation_renderer_clear(renderer);
        break;

    case ANNOT_TEXT:
        if (renderer->text_count < ANNOT_RENDERER_MAX_TEXTS) {
            text_label_t *tl = &renderer->texts[renderer->text_count++];
            tl->pos       = event->text.pos;
            tl->color     = event->text.color;
            tl->font_size = event->text.font_size;
            tl->text_len  = event->text.text_len;
            memcpy(tl->text, event->text.text, event->text.text_len);
            tl->text[event->text.text_len] = '\0';
        }
        break;

    case ANNOT_POINTER_MOVE:
    case ANNOT_POINTER_HIDE:
        /* Handled by pointer_sync module */
        break;

    default:
        break;
    }
}

/* ── Compositing ─────────────────────────────────────────────────── */

/* Porter-Duff src-over blend of ARGB colour onto RGBA pixel */
static void blend_pixel(uint8_t *rgba, annot_color_t color) {
    uint8_t sr = (color >> 16) & 0xFF;
    uint8_t sg = (color >>  8) & 0xFF;
    uint8_t sb = (color      ) & 0xFF;
    uint8_t sa = (color >> 24) & 0xFF;

    if (sa == 0) return;
    if (sa == 255) {
        rgba[0] = sr; rgba[1] = sg; rgba[2] = sb; rgba[3] = 255;
        return;
    }

    float a = sa / 255.0f;
    float inv_a = 1.0f - a;
    rgba[0] = (uint8_t)(sr * a + rgba[0] * inv_a);
    rgba[1] = (uint8_t)(sg * a + rgba[1] * inv_a);
    rgba[2] = (uint8_t)(sb * a + rgba[2] * inv_a);
    rgba[3] = (uint8_t)(sa + rgba[3] * inv_a);
}

/* Draw a filled circle at (px,py) with radius r */
static void draw_circle(uint8_t *pixels, int width, int height, int stride,
                         int cx, int cy, int r, annot_color_t color) {
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx*dx + dy*dy > r*r) continue;
            int x = cx + dx;
            int y = cy + dy;
            if (x < 0 || x >= width || y < 0 || y >= height) continue;
            blend_pixel(pixels + y * stride + x * 4, color);
        }
    }
}

/* Draw a line from (x0,y0) to (x1,y1) with thickness 2*r */
static void draw_line(uint8_t *pixels, int width, int height, int stride,
                       int x0, int y0, int x1, int y1,
                       int r, annot_color_t color) {
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;

    while (1) {
        draw_circle(pixels, width, height, stride, x0, y0, r, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 <  dx) { err += dx; y0 += sy; }
    }
}

void annotation_renderer_composite(annotation_renderer_t *renderer,
                                   uint8_t               *pixels,
                                   int                    width,
                                   int                    height,
                                   int                    stride) {
    if (!renderer || !pixels || width <= 0 || height <= 0) return;

    /* Draw strokes */
    for (int i = 0; i < renderer->stroke_count; i++) {
        stroke_t *s = &renderer->strokes[i];
        if (s->count < 1) continue;

        int r = (int)(s->width * (float)width / 1000.0f);
        if (r < 1) r = 1;

        for (int j = 0; j < s->count; j++) {
            int px = (int)(s->points[j].x * (float)width);
            int py = (int)(s->points[j].y * (float)height);

            if (j == 0) {
                draw_circle(pixels, width, height, stride, px, py, r, s->color);
            } else {
                int px0 = (int)(s->points[j-1].x * (float)width);
                int py0 = (int)(s->points[j-1].y * (float)height);
                draw_line(pixels, width, height, stride,
                           px0, py0, px, py, r, s->color);
            }
        }
    }

    /* Draw text labels (simple stamp: mark a small box at position) */
    for (int i = 0; i < renderer->text_count; i++) {
        text_label_t *tl = &renderer->texts[i];
        int tx = (int)(tl->pos.x * (float)width);
        int ty = (int)(tl->pos.y * (float)height);
        int box = (int)(tl->font_size * 0.5f);
        if (box < 2) box = 2;
        draw_circle(pixels, width, height, stride, tx, ty, box, tl->color);
    }
}
