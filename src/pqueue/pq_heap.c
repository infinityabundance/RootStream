/*
 * pq_heap.c — Binary min-heap priority queue
 */

#include "pq_heap.h"
#include <stdlib.h>
#include <string.h>

struct pq_heap_s {
    pq_entry_t data[PQ_MAX_SIZE];
    int        count;
};

pq_heap_t *pq_heap_create(void) {
    return calloc(1, sizeof(pq_heap_t));
}

void pq_heap_destroy(pq_heap_t *h) { free(h); }

void pq_heap_clear(pq_heap_t *h) { if (h) h->count = 0; }

int pq_heap_count(const pq_heap_t *h) { return h ? h->count : 0; }

/* Sift up: bubble element at idx toward root while smaller than parent */
static void sift_up(pq_entry_t *data, int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (data[parent].key <= data[idx].key) break;
        pq_entry_t tmp = data[parent];
        data[parent] = data[idx];
        data[idx]    = tmp;
        idx = parent;
    }
}

/* Sift down: push element at idx toward leaves */
static void sift_down(pq_entry_t *data, int count, int idx) {
    while (1) {
        int smallest = idx;
        int l = 2 * idx + 1, r = 2 * idx + 2;
        if (l < count && data[l].key < data[smallest].key) smallest = l;
        if (r < count && data[r].key < data[smallest].key) smallest = r;
        if (smallest == idx) break;
        pq_entry_t tmp    = data[smallest];
        data[smallest]    = data[idx];
        data[idx]         = tmp;
        idx = smallest;
    }
}

int pq_heap_push(pq_heap_t *h, const pq_entry_t *e) {
    if (!h || !e || h->count >= PQ_MAX_SIZE) return -1;
    h->data[h->count] = *e;
    sift_up(h->data, h->count);
    h->count++;
    return 0;
}

int pq_heap_pop(pq_heap_t *h, pq_entry_t *out) {
    if (!h || !out || h->count == 0) return -1;
    *out = h->data[0];
    h->count--;
    if (h->count > 0) {
        h->data[0] = h->data[h->count];
        sift_down(h->data, h->count, 0);
    }
    return 0;
}

int pq_heap_peek(const pq_heap_t *h, pq_entry_t *out) {
    if (!h || !out || h->count == 0) return -1;
    *out = h->data[0];
    return 0;
}
