/*
 * session_checkpoint.c — Checkpoint save/load implementation
 */

#include "session_checkpoint.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

struct checkpoint_manager_s {
    char dir[CHECKPOINT_DIR_MAX];
    int  max_keep;
    uint64_t seq;   /* per-manager sequence counter */
};

checkpoint_manager_t *checkpoint_manager_create(
        const checkpoint_config_t *config) {
    checkpoint_manager_t *m = calloc(1, sizeof(*m));
    if (!m) return NULL;

    if (config) {
        strncpy(m->dir, config->dir, CHECKPOINT_DIR_MAX - 1);
        m->max_keep = (config->max_keep > 0) ? config->max_keep
                                              : CHECKPOINT_MAX_KEEP;
    } else {
        strncpy(m->dir, "/tmp", CHECKPOINT_DIR_MAX - 1);
        m->max_keep = CHECKPOINT_MAX_KEEP;
    }
    m->seq = 1;
    return m;
}

void checkpoint_manager_destroy(checkpoint_manager_t *mgr) {
    free(mgr);
}

/* Build canonical checkpoint filename */
static void ckpt_filename(char *out, size_t outsz,
                           const char *dir,
                           uint64_t session_id,
                           uint64_t seq) {
    snprintf(out, outsz, "%s/rootstream-ckpt-%llu-%llu.bin",
             dir,
             (unsigned long long)session_id,
             (unsigned long long)seq);
}

int checkpoint_save(checkpoint_manager_t  *mgr,
                    const session_state_t *state) {
    if (!mgr || !state) return -1;

    uint8_t buf[SESSION_STATE_MAX_SIZE];
    int n = session_state_serialise(state, buf, sizeof(buf));
    if (n < 0) return -1;

    /* Write to temp file, then rename for atomicity */
    char tmp_path[CHECKPOINT_DIR_MAX + 64];
    snprintf(tmp_path, sizeof(tmp_path), "%s/.ckpt_tmp_%llu.bin",
             mgr->dir, (unsigned long long)state->session_id);

    FILE *f = fopen(tmp_path, "wb");
    if (!f) return -1;
    if (fwrite(buf, 1, (size_t)n, f) != (size_t)n) {
        fclose(f);
        remove(tmp_path);
        return -1;
    }
    fclose(f);

    char final_path[CHECKPOINT_DIR_MAX + 64];
    ckpt_filename(final_path, sizeof(final_path),
                  mgr->dir, state->session_id, mgr->seq++);

    if (rename(tmp_path, final_path) != 0) {
        remove(tmp_path);
        return -1;
    }

    return 0;
}

int checkpoint_load(const checkpoint_manager_t *mgr,
                    uint64_t                    session_id,
                    session_state_t            *state) {
    if (!mgr || !state) return -1;

    /* Find the highest-seq checkpoint for this session */
    char prefix[128];
    snprintf(prefix, sizeof(prefix), "rootstream-ckpt-%llu-",
             (unsigned long long)session_id);

    DIR *d = opendir(mgr->dir);
    if (!d) return -1;

    char best_name[256] = {0};
    uint64_t best_seq = 0;
    struct dirent *ent;

    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, prefix, strlen(prefix)) != 0) continue;
        /* Parse seq from suffix */
        const char *seq_start = ent->d_name + strlen(prefix);
        uint64_t seq = (uint64_t)strtoull(seq_start, NULL, 10);
        if (seq >= best_seq) {
            best_seq = seq;
            snprintf(best_name, sizeof(best_name), "%s", ent->d_name);
        }
    }
    closedir(d);

    if (best_name[0] == '\0') return -1;

    char path[CHECKPOINT_DIR_MAX + 256];
    snprintf(path, sizeof(path), "%s/%s", mgr->dir, best_name);

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    uint8_t buf[SESSION_STATE_MAX_SIZE];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    if (n < SESSION_STATE_MIN_SIZE) return -1;
    return session_state_deserialise(buf, n, state);
}

int checkpoint_delete(checkpoint_manager_t *mgr, uint64_t session_id) {
    if (!mgr) return 0;

    char prefix[128];
    snprintf(prefix, sizeof(prefix), "rootstream-ckpt-%llu-",
             (unsigned long long)session_id);

    DIR *d = opendir(mgr->dir);
    if (!d) return 0;

    int deleted = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, prefix, strlen(prefix)) != 0) continue;
        char path[CHECKPOINT_DIR_MAX + 256];
        snprintf(path, sizeof(path), "%s/%s", mgr->dir, ent->d_name);
        if (remove(path) == 0) deleted++;
    }
    closedir(d);
    return deleted;
}

bool checkpoint_exists(const checkpoint_manager_t *mgr,
                       uint64_t                    session_id) {
    if (!mgr) return false;

    char prefix[128];
    snprintf(prefix, sizeof(prefix), "rootstream-ckpt-%llu-",
             (unsigned long long)session_id);

    DIR *d = opendir(mgr->dir);
    if (!d) return false;

    bool found = false;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strncmp(ent->d_name, prefix, strlen(prefix)) == 0) {
            found = true;
            break;
        }
    }
    closedir(d);
    return found;
}
