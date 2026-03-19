/*
 * schedule_store.c — Binary schedule persistence implementation
 */

#include "schedule_store.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scheduler.h"

static void w16le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}
static void w32le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}
static uint16_t r16le(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static uint32_t r32le(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

int schedule_store_save(const char *path, const schedule_entry_t *entries, size_t n) {
    if (!path || !entries)
        return -1;

    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    FILE *f = fopen(tmp, "wb");
    if (!f)
        return -1;

    /* File header */
    uint8_t hdr[SCHEDULE_STORE_HDR_SZ];
    w32le(hdr + 0, (uint32_t)SCHEDULE_STORE_MAGIC);
    w16le(hdr + 4, SCHEDULE_STORE_VERSION);
    w16le(hdr + 6, (uint16_t)n);
    w32le(hdr + 8, 0);

    if (fwrite(hdr, 1, SCHEDULE_STORE_HDR_SZ, f) != SCHEDULE_STORE_HDR_SZ) {
        fclose(f);
        remove(tmp);
        return -1;
    }

    /* Entries */
    for (size_t i = 0; i < n; i++) {
        uint8_t buf[SCHEDULE_ENTRY_MAX_SZ];
        int esz = schedule_entry_encode(&entries[i], buf, sizeof(buf));
        if (esz < 0) {
            fclose(f);
            remove(tmp);
            return -1;
        }

        uint8_t len[2];
        w16le(len, (uint16_t)esz);
        if (fwrite(len, 1, 2, f) != 2 || fwrite(buf, 1, (size_t)esz, f) != (size_t)esz) {
            fclose(f);
            remove(tmp);
            return -1;
        }
    }

    fclose(f);
    return rename(tmp, path) == 0 ? 0 : -1;
}

int schedule_store_load(const char *path, schedule_entry_t *entries, size_t max,
                        size_t *out_count) {
    if (!path || !entries || !out_count)
        return -1;

    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;

    uint8_t hdr[SCHEDULE_STORE_HDR_SZ];
    if (fread(hdr, 1, SCHEDULE_STORE_HDR_SZ, f) != SCHEDULE_STORE_HDR_SZ) {
        fclose(f);
        return -1;
    }
    if (r32le(hdr) != (uint32_t)SCHEDULE_STORE_MAGIC) {
        fclose(f);
        return -1;
    }
    if (r16le(hdr + 4) != SCHEDULE_STORE_VERSION) {
        fclose(f);
        return -1;
    }

    size_t count = r16le(hdr + 6);
    if (count > max)
        count = max;

    size_t loaded = 0;
    for (size_t i = 0; i < count; i++) {
        uint8_t len_buf[2];
        if (fread(len_buf, 1, 2, f) != 2)
            break;
        uint16_t esz = r16le(len_buf);
        if (esz > SCHEDULE_ENTRY_MAX_SZ)
            break;

        uint8_t buf[SCHEDULE_ENTRY_MAX_SZ];
        if (fread(buf, 1, esz, f) != esz)
            break;
        if (schedule_entry_decode(buf, esz, &entries[loaded]) == 0)
            loaded++;
    }

    fclose(f);
    *out_count = loaded;
    return 0;
}
