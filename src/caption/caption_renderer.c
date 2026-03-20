/*
 * caption_renderer.c — Caption overlay compositor implementation
 *
 * Draws caption text using a built-in 5×7 pixel bitmap font.
 * Background pill rendered with Porter-Duff src-over alpha blending.
 */

#include "caption_renderer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── 5×7 bitmap font (ASCII 32–127) ──────────────────────────────
 * Each glyph is 5 bytes, one byte per row (top→bottom), bit4 = leftmost.
 */
static const uint8_t FONT5X7[96][5] = {
    /* ' ' */ {0x00, 0x00, 0x00, 0x00, 0x00},
    /* '!' */ {0x00, 0x5F, 0x00, 0x00, 0x00},
    /* '"' */ {0x07, 0x00, 0x07, 0x00, 0x00},
    /* '#' */ {0x14, 0x7F, 0x14, 0x7F, 0x14},
    /* '$' */ {0x24, 0x2A, 0x7F, 0x2A, 0x12},
    /* '%' */ {0x23, 0x13, 0x08, 0x64, 0x62},
    /* '&' */ {0x36, 0x49, 0x55, 0x22, 0x50},
    /* '\''*/ {0x00, 0x05, 0x03, 0x00, 0x00},
    /* '(' */ {0x00, 0x1C, 0x22, 0x41, 0x00},
    /* ')' */ {0x00, 0x41, 0x22, 0x1C, 0x00},
    /* '*' */ {0x14, 0x08, 0x3E, 0x08, 0x14},
    /* '+' */ {0x08, 0x08, 0x3E, 0x08, 0x08},
    /* ',' */ {0x00, 0x50, 0x30, 0x00, 0x00},
    /* '-' */ {0x08, 0x08, 0x08, 0x08, 0x08},
    /* '.' */ {0x00, 0x60, 0x60, 0x00, 0x00},
    /* '/' */ {0x20, 0x10, 0x08, 0x04, 0x02},
    /* '0' */ {0x3E, 0x51, 0x49, 0x45, 0x3E},
    /* '1' */ {0x00, 0x42, 0x7F, 0x40, 0x00},
    /* '2' */ {0x42, 0x61, 0x51, 0x49, 0x46},
    /* '3' */ {0x21, 0x41, 0x45, 0x4B, 0x31},
    /* '4' */ {0x18, 0x14, 0x12, 0x7F, 0x10},
    /* '5' */ {0x27, 0x45, 0x45, 0x45, 0x39},
    /* '6' */ {0x3C, 0x4A, 0x49, 0x49, 0x30},
    /* '7' */ {0x01, 0x71, 0x09, 0x05, 0x03},
    /* '8' */ {0x36, 0x49, 0x49, 0x49, 0x36},
    /* '9' */ {0x06, 0x49, 0x49, 0x29, 0x1E},
    /* ':' */ {0x00, 0x36, 0x36, 0x00, 0x00},
    /* ';' */ {0x00, 0x56, 0x36, 0x00, 0x00},
    /* '<' */ {0x08, 0x14, 0x22, 0x41, 0x00},
    /* '=' */ {0x14, 0x14, 0x14, 0x14, 0x14},
    /* '>' */ {0x00, 0x41, 0x22, 0x14, 0x08},
    /* '?' */ {0x02, 0x01, 0x51, 0x09, 0x06},
    /* '@' */ {0x32, 0x49, 0x79, 0x41, 0x3E},
    /* 'A' */ {0x7E, 0x11, 0x11, 0x11, 0x7E},
    /* 'B' */ {0x7F, 0x49, 0x49, 0x49, 0x36},
    /* 'C' */ {0x3E, 0x41, 0x41, 0x41, 0x22},
    /* 'D' */ {0x7F, 0x41, 0x41, 0x22, 0x1C},
    /* 'E' */ {0x7F, 0x49, 0x49, 0x49, 0x41},
    /* 'F' */ {0x7F, 0x09, 0x09, 0x09, 0x01},
    /* 'G' */ {0x3E, 0x41, 0x49, 0x49, 0x7A},
    /* 'H' */ {0x7F, 0x08, 0x08, 0x08, 0x7F},
    /* 'I' */ {0x00, 0x41, 0x7F, 0x41, 0x00},
    /* 'J' */ {0x20, 0x40, 0x41, 0x3F, 0x01},
    /* 'K' */ {0x7F, 0x08, 0x14, 0x22, 0x41},
    /* 'L' */ {0x7F, 0x40, 0x40, 0x40, 0x40},
    /* 'M' */ {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    /* 'N' */ {0x7F, 0x04, 0x08, 0x10, 0x7F},
    /* 'O' */ {0x3E, 0x41, 0x41, 0x41, 0x3E},
    /* 'P' */ {0x7F, 0x09, 0x09, 0x09, 0x06},
    /* 'Q' */ {0x3E, 0x41, 0x51, 0x21, 0x5E},
    /* 'R' */ {0x7F, 0x09, 0x19, 0x29, 0x46},
    /* 'S' */ {0x46, 0x49, 0x49, 0x49, 0x31},
    /* 'T' */ {0x01, 0x01, 0x7F, 0x01, 0x01},
    /* 'U' */ {0x3F, 0x40, 0x40, 0x40, 0x3F},
    /* 'V' */ {0x1F, 0x20, 0x40, 0x20, 0x1F},
    /* 'W' */ {0x3F, 0x40, 0x38, 0x40, 0x3F},
    /* 'X' */ {0x63, 0x14, 0x08, 0x14, 0x63},
    /* 'Y' */ {0x07, 0x08, 0x70, 0x08, 0x07},
    /* 'Z' */ {0x61, 0x51, 0x49, 0x45, 0x43},
    /* '[' */ {0x00, 0x7F, 0x41, 0x41, 0x00},
    /* '\\'*/ {0x02, 0x04, 0x08, 0x10, 0x20},
    /* ']' */ {0x00, 0x41, 0x41, 0x7F, 0x00},
    /* '^' */ {0x04, 0x02, 0x01, 0x02, 0x04},
    /* '_' */ {0x40, 0x40, 0x40, 0x40, 0x40},
    /* '`' */ {0x00, 0x01, 0x02, 0x04, 0x00},
    /* 'a' */ {0x20, 0x54, 0x54, 0x54, 0x78},
    /* 'b' */ {0x7F, 0x48, 0x44, 0x44, 0x38},
    /* 'c' */ {0x38, 0x44, 0x44, 0x44, 0x20},
    /* 'd' */ {0x38, 0x44, 0x44, 0x48, 0x7F},
    /* 'e' */ {0x38, 0x54, 0x54, 0x54, 0x18},
    /* 'f' */ {0x08, 0x7E, 0x09, 0x01, 0x02},
    /* 'g' */ {0x0C, 0x52, 0x52, 0x52, 0x3E},
    /* 'h' */ {0x7F, 0x08, 0x04, 0x04, 0x78},
    /* 'i' */ {0x00, 0x44, 0x7D, 0x40, 0x00},
    /* 'j' */ {0x20, 0x40, 0x44, 0x3D, 0x00},
    /* 'k' */ {0x7F, 0x10, 0x28, 0x44, 0x00},
    /* 'l' */ {0x00, 0x41, 0x7F, 0x40, 0x00},
    /* 'm' */ {0x7C, 0x04, 0x18, 0x04, 0x78},
    /* 'n' */ {0x7C, 0x08, 0x04, 0x04, 0x78},
    /* 'o' */ {0x38, 0x44, 0x44, 0x44, 0x38},
    /* 'p' */ {0x7C, 0x14, 0x14, 0x14, 0x08},
    /* 'q' */ {0x08, 0x14, 0x14, 0x18, 0x7C},
    /* 'r' */ {0x7C, 0x08, 0x04, 0x04, 0x08},
    /* 's' */ {0x48, 0x54, 0x54, 0x54, 0x20},
    /* 't' */ {0x04, 0x3F, 0x44, 0x40, 0x20},
    /* 'u' */ {0x3C, 0x40, 0x40, 0x20, 0x7C},
    /* 'v' */ {0x1C, 0x20, 0x40, 0x20, 0x1C},
    /* 'w' */ {0x3C, 0x40, 0x30, 0x40, 0x3C},
    /* 'x' */ {0x44, 0x28, 0x10, 0x28, 0x44},
    /* 'y' */ {0x0C, 0x50, 0x50, 0x50, 0x3C},
    /* 'z' */ {0x44, 0x64, 0x54, 0x4C, 0x44},
    /* '{' */ {0x00, 0x08, 0x36, 0x41, 0x00},
    /* '|' */ {0x00, 0x00, 0x7F, 0x00, 0x00},
    /* '}' */ {0x00, 0x41, 0x36, 0x08, 0x00},
    /* '~' */ {0x10, 0x08, 0x08, 0x10, 0x08},
    /* DEL */ {0x00, 0x00, 0x00, 0x00, 0x00},
};

/* ── Pixel blend ───────────────────────────────────────────────── */

static void blend(uint8_t *rgba, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (a == 0)
        return;
    if (a == 255) {
        rgba[0] = r;
        rgba[1] = g;
        rgba[2] = b;
        rgba[3] = 255;
        return;
    }
    float fa = a / 255.0f;
    float fi = 1.0f - fa;
    rgba[0] = (uint8_t)(r * fa + rgba[0] * fi);
    rgba[1] = (uint8_t)(g * fa + rgba[1] * fi);
    rgba[2] = (uint8_t)(b * fa + rgba[2] * fi);
    rgba[3] = (uint8_t)(a + rgba[3] * fi);
}

/* ── Renderer struct ───────────────────────────────────────────── */

struct caption_renderer_s {
    uint32_t bg_color;
    uint32_t fg_color;
    int font_scale;
    int margin_px;
};

caption_renderer_t *caption_renderer_create(const caption_renderer_config_t *config) {
    caption_renderer_t *r = calloc(1, sizeof(*r));
    if (!r)
        return NULL;

    if (config) {
        r->bg_color = config->bg_color;
        r->fg_color = config->fg_color;
        r->font_scale =
            (config->font_scale >= 1 && config->font_scale <= 4) ? config->font_scale : 2;
        r->margin_px = (config->margin_px >= 0) ? config->margin_px : 8;
    } else {
        r->bg_color = 0xBB000000;
        r->fg_color = 0xFFFFFFFF;
        r->font_scale = 2;
        r->margin_px = 8;
    }
    return r;
}

void caption_renderer_destroy(caption_renderer_t *r) {
    free(r);
}

/* ── Draw a single glyph at (px, py) ──────────────────────────── */

static void draw_glyph(uint8_t *pixels, int width, int height, int stride, int px, int py,
                       unsigned char ch, uint8_t fr, uint8_t fgn, uint8_t fb, uint8_t fa,
                       int scale) {
    if (ch < 32 || ch > 127)
        ch = '?';
    const uint8_t *glyph = FONT5X7[ch - 32];

    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (!(glyph[row] & (0x10 >> col)))
                continue;
            for (int sy = 0; sy < scale; sy++) {
                for (int sx = 0; sx < scale; sx++) {
                    int x = px + col * scale + sx;
                    int y = py + row * scale + sy;
                    if (x < 0 || x >= width || y < 0 || y >= height)
                        continue;
                    blend(pixels + y * stride + x * 4, fr, fgn, fb, fa);
                }
            }
        }
    }
}

/* ── Draw caption event ────────────────────────────────────────── */

static void draw_caption(caption_renderer_t *r, uint8_t *pixels, int width, int height, int stride,
                         const caption_event_t *event) {
    int scale = r->font_scale;
    int glyph_w = 6 * scale; /* 5 px + 1 spacing */
    int glyph_h = 8 * scale; /* 7 px + 1 spacing */
    int margin = r->margin_px;

    int text_px_w = (int)event->text_len * glyph_w;
    int box_w = text_px_w + margin * 2;
    int box_h = glyph_h + margin * 2;

    /* Determine Y: bottom-anchored by default */
    int rows_total = 15;
    int row = (event->row < rows_total) ? (int)event->row : (rows_total - 1);
    int y_top;
    if (event->flags & CAPTION_FLAG_TOP) {
        y_top = margin + row * (box_h + 2);
    } else {
        y_top = height - margin - box_h - row * (box_h + 2);
    }

    /* Centre horizontally */
    int x_left = (width - box_w) / 2;
    if (x_left < 0)
        x_left = 0;

    /* Extract colour components */
    uint8_t bg_a = (uint8_t)(r->bg_color >> 24);
    uint8_t bg_r = (uint8_t)(r->bg_color >> 16);
    uint8_t bg_g = (uint8_t)(r->bg_color >> 8);
    uint8_t bg_b = (uint8_t)(r->bg_color);

    uint8_t fg_a = (uint8_t)(r->fg_color >> 24);
    uint8_t fg_r = (uint8_t)(r->fg_color >> 16);
    uint8_t fg_gn = (uint8_t)(r->fg_color >> 8);
    uint8_t fg_b = (uint8_t)(r->fg_color);

    /* Draw background box */
    for (int y = y_top; y < y_top + box_h; y++) {
        if (y < 0 || y >= height)
            continue;
        for (int x = x_left; x < x_left + box_w; x++) {
            if (x < 0 || x >= width)
                continue;
            blend(pixels + y * stride + x * 4, bg_r, bg_g, bg_b, bg_a);
        }
    }

    /* Draw glyphs */
    int gx = x_left + margin;
    int gy = y_top + margin;
    for (int i = 0; i < (int)event->text_len; i++) {
        draw_glyph(pixels, width, height, stride, gx + i * glyph_w, gy,
                   (unsigned char)event->text[i], fg_r, fg_gn, fg_b, fg_a, scale);
    }
}

/* ── Public entry point ────────────────────────────────────────── */

int caption_renderer_draw(caption_renderer_t *r, uint8_t *pixels, int width, int height, int stride,
                          const caption_event_t *events, int n, uint64_t now_us) {
    if (!r || !pixels || !events || width <= 0 || height <= 0)
        return 0;

    int rendered = 0;
    for (int i = 0; i < n; i++) {
        if (!caption_event_is_active(&events[i], now_us))
            continue;
        draw_caption(r, pixels, width, height, stride, &events[i]);
        rendered++;
    }
    return rendered;
}
