/*
 * ts_writer.c — Minimal MPEG-TS segment writer implementation
 *
 * Generates a standards-compliant single-program TS stream suitable
 * for HLS segments.  Only the bare minimum tables and PES framing are
 * implemented; no conditional access or multiple programs.
 */

#include "ts_writer.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define VIDEO_PID   0x100   /* 256 */
#define PMT_PID     0x1000  /* 4096 */
#define PROGRAM_NUM 1

struct ts_writer_s {
    int    fd;
    size_t bytes_written;
    uint8_t continuity[0x2000];  /* per-PID continuity counters */
};

ts_writer_t *ts_writer_create(int fd) {
    ts_writer_t *w = calloc(1, sizeof(*w));
    if (!w) return NULL;
    w->fd = fd;
    return w;
}

void ts_writer_destroy(ts_writer_t *w) {
    free(w);
}

size_t ts_writer_bytes_written(const ts_writer_t *w) {
    return w ? w->bytes_written : 0;
}

/* ── Internal helpers ─────────────────────────────────────────────── */

static void set_u16be(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8);
    p[1] = (uint8_t)(v);
}

static uint32_t crc32_mpeg(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint32_t)data[i] << 24;
        for (int b = 0; b < 8; b++) {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ 0x04C11DB7;
            else
                crc <<= 1;
        }
    }
    return crc;
}

static int write_ts_packet(ts_writer_t *w,
                            uint16_t     pid,
                            bool         payload_unit_start,
                            bool         random_access,
                            const uint8_t *data,
                            size_t        data_len) {
    uint8_t pkt[HLS_TS_PACKET_SZ];
    memset(pkt, 0xFF, sizeof(pkt));

    int stuffing = (int)HLS_TS_PACKET_SZ - 4 - (int)data_len;

    /* Adaptation field needed for stuffing or random_access flag */
    bool has_af = (stuffing > 0) || random_access;
    uint8_t af_len = has_af ? (uint8_t)(stuffing > 0 ? stuffing - 1 : 0) : 0;

    uint8_t flags = 0;
    flags |= (uint8_t)(payload_unit_start ? 0x40 : 0x00);
    flags |= (uint8_t)(has_af ? 0x20 : 0x00);  /* adaptation field flag */
    flags |= 0x10;                               /* payload present */
    flags |= (w->continuity[pid] & 0x0F);
    w->continuity[pid] = (w->continuity[pid] + 1) & 0x0F;

    pkt[0] = HLS_TS_SYNC_BYTE;
    pkt[1] = (uint8_t)((pid >> 8) & 0x1F);
    pkt[2] = (uint8_t)(pid & 0xFF);
    pkt[3] = flags;

    int pos = 4;
    if (has_af) {
        pkt[pos++] = af_len;
        if (af_len > 0) {
            pkt[pos++] = random_access ? 0x40 : 0x00; /* RAI flag */
            /* rest already filled with 0xFF (stuffing) */
            pos += af_len - 1;
        }
    }

    if (data && data_len > 0 && pos + (int)data_len <= HLS_TS_PACKET_SZ)
        memcpy(pkt + pos, data, data_len);

    ssize_t written = write(w->fd, pkt, HLS_TS_PACKET_SZ);
    if (written != HLS_TS_PACKET_SZ) return -1;
    w->bytes_written += HLS_TS_PACKET_SZ;
    return 0;
}

/* ── PAT ─────────────────────────────────────────────────────────── */

int ts_writer_write_pat_pmt(ts_writer_t *w) {
    if (!w) return -1;

    /* PAT: 8 bytes + 4 CRC */
    uint8_t pat[12];
    pat[0] = 0x00; /* table_id */
    set_u16be(pat + 1, 0xB00D); /* section_syntax + length=13 */
    set_u16be(pat + 3, 0x0001); /* transport_stream_id */
    pat[5] = 0xC1; /* version=0, current=1 */
    pat[6] = 0x00; /* section_number */
    pat[7] = 0x00; /* last_section_number */
    /* program 1 → PMT PID */
    set_u16be(pat + 8,  PROGRAM_NUM);
    set_u16be(pat + 10, 0xE000 | PMT_PID);
    uint32_t crc = crc32_mpeg(pat, 12);
    uint8_t pat_full[17];
    pat_full[0] = 0x00; /* pointer_field */
    memcpy(pat_full + 1, pat, 12);
    pat_full[13] = (uint8_t)(crc >> 24);
    pat_full[14] = (uint8_t)(crc >> 16);
    pat_full[15] = (uint8_t)(crc >>  8);
    pat_full[16] = (uint8_t)(crc      );

    if (write_ts_packet(w, 0, true, false, pat_full, 17) != 0) return -1;

    /* PMT: video stream only */
    uint8_t pmt[13];
    pmt[0] = 0x02; /* table_id */
    set_u16be(pmt + 1, 0xB00F); /* section_syntax + length=15 */
    set_u16be(pmt + 3, PROGRAM_NUM);
    pmt[5] = 0xC1;
    pmt[6] = 0x00;
    pmt[7] = 0x00;
    set_u16be(pmt + 8, 0xE100 | VIDEO_PID); /* PCR_PID */
    set_u16be(pmt + 10, 0xF000);            /* program_info_length=0 */
    /* stream descriptor: type=0x1B (H.264), PID=VIDEO_PID */
    pmt[12] = 0x1B;
    uint8_t pmt2[4];
    set_u16be(pmt2 + 0, 0xE000 | VIDEO_PID);
    set_u16be(pmt2 + 2, 0xF000); /* ES_info_length=0 */

    uint8_t pmt_all[21];
    memcpy(pmt_all,      pmt,  13);
    memcpy(pmt_all + 13, pmt2,  4);
    uint32_t pmt_crc = crc32_mpeg(pmt_all, 17);
    uint8_t pmt_full[23];
    pmt_full[0] = 0x00;
    memcpy(pmt_full + 1, pmt_all, 17);
    pmt_full[18] = (uint8_t)(pmt_crc >> 24);
    pmt_full[19] = (uint8_t)(pmt_crc >> 16);
    pmt_full[20] = (uint8_t)(pmt_crc >>  8);
    pmt_full[21] = (uint8_t)(pmt_crc      );

    return write_ts_packet(w, PMT_PID, true, false, pmt_full, 22);
}

/* ── PES ─────────────────────────────────────────────────────────── */

int ts_writer_write_pes(ts_writer_t   *w,
                         const uint8_t *payload,
                         size_t         payload_len,
                         uint64_t       pts_90khz,
                         bool           is_keyframe) {
    if (!w || !payload || payload_len == 0) return -1;

    /* Build PES header */
    uint8_t pes[14];
    /* start code + stream_id */
    pes[0] = 0x00; pes[1] = 0x00; pes[2] = 0x01;
    pes[3] = 0xE0; /* stream_id: video */
    /* PES packet length: 0 = unbounded for video */
    pes[4] = 0x00; pes[5] = 0x00;
    pes[6] = 0x80; /* marker + no scrambling */
    pes[7] = 0x80; /* PTS present */
    pes[8] = 0x05; /* PES header data length */
    /* PTS encoding */
    uint8_t pts4 = (uint8_t)(0x21 | (((pts_90khz >> 30) & 0x07) << 1));
    uint8_t pts3 = (uint8_t)((pts_90khz >> 22) & 0xFF);
    uint8_t pts2 = (uint8_t)(0x01 | (((pts_90khz >> 15) & 0x7F) << 1));
    uint8_t pts1 = (uint8_t)((pts_90khz >> 7) & 0xFF);
    uint8_t pts0 = (uint8_t)(0x01 | ((pts_90khz & 0x7F) << 1));
    pes[9]  = pts4; pes[10] = pts3; pes[11] = pts2;
    pes[12] = pts1; pes[13] = pts0;

    /* First TS packet carries PES header + as much payload as fits */
    uint8_t first[HLS_TS_PACKET_SZ - 4]; /* 184 bytes */
    memcpy(first, pes, 14);
    size_t first_payload = HLS_TS_PACKET_SZ - 4 - 14;
    if (first_payload > payload_len) first_payload = payload_len;
    memcpy(first + 14, payload, first_payload);
    size_t first_sz = 14 + first_payload;

    if (write_ts_packet(w, VIDEO_PID, true, is_keyframe, first, first_sz) != 0)
        return -1;

    /* Remaining payload in continuation TS packets */
    size_t offset = first_payload;
    while (offset < payload_len) {
        size_t chunk = payload_len - offset;
        if (chunk > HLS_TS_PACKET_SZ - 4) chunk = HLS_TS_PACKET_SZ - 4;
        if (write_ts_packet(w, VIDEO_PID, false, false,
                             payload + offset, chunk) != 0) return -1;
        offset += chunk;
    }

    return 0;
}
