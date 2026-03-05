/*
 * session_state.c — Session state serialisation implementation
 */

#include "session_state.h"

#include <string.h>
#include <stdio.h>

/* ── Write helpers ─────────────────────────────────────────────────── */

static void w16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
}
static void w32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}
static void w64(uint8_t *p, uint64_t v) {
    for (int i = 0; i < 8; i++) p[i] = (uint8_t)(v >> (i * 8));
}

static uint16_t r16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t r32(const uint8_t *p) {
    return (uint32_t)p[0]
         | ((uint32_t)p[1] <<  8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}
static uint64_t r64(const uint8_t *p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= ((uint64_t)p[i] << (i * 8));
    return v;
}

/* ── Public API ─────────────────────────────────────────────────────── */

size_t session_state_serialised_size(const session_state_t *state) {
    if (!state) return 0;
    size_t peer_len = strlen(state->peer_addr);
    if (peer_len > SESSION_MAX_PEER_ADDR) peer_len = SESSION_MAX_PEER_ADDR;
    return SESSION_STATE_MIN_SIZE + peer_len;
}

int session_state_serialise(const session_state_t *state,
                             uint8_t               *buf,
                             size_t                 buf_sz) {
    if (!state || !buf) return -1;

    size_t needed = session_state_serialised_size(state);
    if (buf_sz < needed) return -1;

    uint16_t peer_len = (uint16_t)strlen(state->peer_addr);
    if (peer_len > SESSION_MAX_PEER_ADDR) peer_len = SESSION_MAX_PEER_ADDR;

    w32(buf +  0, (uint32_t)SESSION_STATE_MAGIC);
    w16(buf +  4, SESSION_STATE_VERSION);
    w16(buf +  6, state->flags);
    w64(buf +  8, state->session_id);
    w64(buf + 16, state->created_us);
    w32(buf + 24, state->width);
    w32(buf + 28, state->height);
    w32(buf + 32, state->fps_num);
    w32(buf + 36, state->fps_den);
    w32(buf + 40, state->bitrate_kbps);
    w32(buf + 44, state->audio_sample_rate);
    w32(buf + 48, state->audio_channels);
    w32(buf + 52, state->last_keyframe);
    w64(buf + 56, state->frames_sent);
    memcpy(buf + 64, state->stream_key, SESSION_STREAM_KEY_LEN);
    w16(buf + 96, peer_len);
    if (peer_len > 0) {
        memcpy(buf + 98, state->peer_addr, peer_len);
    }

    return (int)needed;
}

int session_state_deserialise(const uint8_t   *buf,
                               size_t           buf_sz,
                               session_state_t *state) {
    if (!buf || !state || buf_sz < SESSION_STATE_MIN_SIZE) return -1;

    uint32_t magic = r32(buf);
    if (magic != (uint32_t)SESSION_STATE_MAGIC) return -1;

    uint16_t version = r16(buf + 4);
    if (version != SESSION_STATE_VERSION) return -1;

    memset(state, 0, sizeof(*state));
    state->flags             = r16(buf +  6);
    state->session_id        = r64(buf +  8);
    state->created_us        = r64(buf + 16);
    state->width             = r32(buf + 24);
    state->height            = r32(buf + 28);
    state->fps_num           = r32(buf + 32);
    state->fps_den           = r32(buf + 36);
    state->bitrate_kbps      = r32(buf + 40);
    state->audio_sample_rate = r32(buf + 44);
    state->audio_channels    = r32(buf + 48);
    state->last_keyframe     = r32(buf + 52);
    state->frames_sent       = r64(buf + 56);
    memcpy(state->stream_key, buf + 64, SESSION_STREAM_KEY_LEN);

    uint16_t peer_len = r16(buf + 96);
    if (peer_len > SESSION_MAX_PEER_ADDR) return -1;
    if (buf_sz < (size_t)(SESSION_STATE_MIN_SIZE + peer_len)) return -1;
    if (peer_len > 0) {
        memcpy(state->peer_addr, buf + 98, peer_len);
    }
    state->peer_addr[peer_len] = '\0';

    return 0;
}
