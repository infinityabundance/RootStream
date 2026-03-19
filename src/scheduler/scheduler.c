/*
 * scheduler.c — Stream scheduler engine implementation
 */

#include "scheduler.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define DAY_US (86400ULL * 1000000ULL)

typedef struct {
    schedule_entry_t entry;
    bool fired;
    bool used;
} sched_slot_t;

struct scheduler_s {
    sched_slot_t slots[SCHEDULER_MAX_ENTRIES];
    uint64_t next_id;
    scheduler_fire_fn fire_fn;
    void *user_data;
    pthread_mutex_t lock;
};

scheduler_t *scheduler_create(scheduler_fire_fn fire_fn, void *user_data) {
    scheduler_t *s = calloc(1, sizeof(*s));
    if (!s)
        return NULL;
    pthread_mutex_init(&s->lock, NULL);
    s->next_id = 1;
    s->fire_fn = fire_fn;
    s->user_data = user_data;
    return s;
}

void scheduler_destroy(scheduler_t *sched) {
    if (!sched)
        return;
    pthread_mutex_destroy(&sched->lock);
    free(sched);
}

uint64_t scheduler_add(scheduler_t *sched, schedule_entry_t *entry) {
    if (!sched || !entry)
        return 0;

    pthread_mutex_lock(&sched->lock);
    int slot = -1;
    for (int i = 0; i < SCHEDULER_MAX_ENTRIES; i++) {
        if (!sched->slots[i].used) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        pthread_mutex_unlock(&sched->lock);
        return 0;
    }

    entry->id = sched->next_id++;
    sched->slots[slot].entry = *entry;
    sched->slots[slot].fired = false;
    sched->slots[slot].used = true;

    pthread_mutex_unlock(&sched->lock);
    return entry->id;
}

int scheduler_remove(scheduler_t *sched, uint64_t id) {
    if (!sched)
        return -1;
    pthread_mutex_lock(&sched->lock);
    for (int i = 0; i < SCHEDULER_MAX_ENTRIES; i++) {
        if (sched->slots[i].used && sched->slots[i].entry.id == id) {
            sched->slots[i].used = false;
            pthread_mutex_unlock(&sched->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&sched->lock);
    return -1;
}

int scheduler_tick(scheduler_t *sched, uint64_t now_us) {
    if (!sched)
        return 0;

    int fired_count = 0;
    pthread_mutex_lock(&sched->lock);
    for (int i = 0; i < SCHEDULER_MAX_ENTRIES; i++) {
        sched_slot_t *slot = &sched->slots[i];
        if (!slot->used || slot->fired)
            continue;
        if (!(slot->entry.flags & SCHED_FLAG_ENABLED))
            continue;
        if (slot->entry.start_us > now_us)
            continue;

        /* Fire */
        fired_count++;
        if (sched->fire_fn) {
            /* Unlock while calling user callback to avoid deadlock */
            schedule_entry_t copy = slot->entry;
            pthread_mutex_unlock(&sched->lock);
            sched->fire_fn(&copy, sched->user_data);
            pthread_mutex_lock(&sched->lock);
            /* Re-verify slot is still valid after unlock */
            if (!sched->slots[i].used)
                continue;
        }

        if (slot->entry.flags & SCHED_FLAG_REPEAT) {
            /* Advance start by 24h */
            slot->entry.start_us += DAY_US;
        } else {
            slot->fired = true;
            slot->used = false;
        }
    }
    pthread_mutex_unlock(&sched->lock);
    return fired_count;
}

size_t scheduler_count(const scheduler_t *sched) {
    if (!sched)
        return 0;
    size_t count = 0;
    pthread_mutex_lock((pthread_mutex_t *)&sched->lock);
    for (int i = 0; i < SCHEDULER_MAX_ENTRIES; i++) {
        if (sched->slots[i].used && !sched->slots[i].fired)
            count++;
    }
    pthread_mutex_unlock((pthread_mutex_t *)&sched->lock);
    return count;
}

int scheduler_get(scheduler_t *sched, uint64_t id, schedule_entry_t *out) {
    if (!sched || !out)
        return -1;
    pthread_mutex_lock(&sched->lock);
    for (int i = 0; i < SCHEDULER_MAX_ENTRIES; i++) {
        if (sched->slots[i].used && sched->slots[i].entry.id == id) {
            *out = sched->slots[i].entry;
            pthread_mutex_unlock(&sched->lock);
            return 0;
        }
    }
    pthread_mutex_unlock(&sched->lock);
    return -1;
}

void scheduler_clear(scheduler_t *sched) {
    if (!sched)
        return;
    pthread_mutex_lock(&sched->lock);
    for (int i = 0; i < SCHEDULER_MAX_ENTRIES; i++) {
        sched->slots[i].used = false;
        sched->slots[i].fired = false;
    }
    pthread_mutex_unlock(&sched->lock);
}
