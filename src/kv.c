#include "jdsimple.h"

#define MAGIC 0x7ab377bb

typedef struct {
    uint32_t key;
    uint32_t value;
} kv_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    kv_t keys[];
} store_t;

#define IS_LAST(kv) ((kv)->key + 1) == 0

#define store ((store_t *)SETTINGS_START)

#define NUM_KV (((SETTINGS_SIZE - sizeof(store_t)) / sizeof(kv_t)) - 3)

static uint32_t num_entries(void) {
    kv_t *kv = store->keys;
    while (!IS_LAST(kv) && kv - store->keys < NUM_KV)
        kv++;
    return kv - store->keys;
}

static void kv_gc(void) {
    target_disable_irq();

    // first erase, so that we have a clear bailout path
    flash_erase(store);

    store_t *tmp = alloc_emergency_area(SETTINGS_SIZE);
    memcpy(tmp, store, SETTINGS_SIZE);
    kv_t *first = tmp->keys;
    kv_t *kv = first;
    uint32_t numdel = 0;
    while (!IS_LAST(kv) && kv - first < NUM_KV) {
        if (kv->key == 0)
            numdel++;
        kv++;
    }
    kv_t *last = kv;
    uint32_t newsize = last - first - numdel;
    if (newsize >= NUM_KV - 1) {
        // whoops, we're out of space even after compression! just bail out
        target_reset();
    }

    kv_t *dst = first;
    for (kv = first; kv < last; kv++) {
        if (kv->key)
            *dst++ = *kv;
    }

    flash_program(store, tmp, (uint8_t *)dst - (uint8_t *)tmp);

    target_reset();
}

void kv_init() {
    // we should never have NUM_KV entries here
    if (store->magic != MAGIC || store->version != 0 || num_entries() >= NUM_KV) {
        flash_erase(store);
        store_t init = {.magic = MAGIC, .version = 0};
        flash_program(store, &init, sizeof(init));
        if (store->magic != MAGIC)
            jd_panic();
    }
}

static kv_t *lookup(uint32_t key) {
    // backwards maybe better for perf?
    for (kv_t *kv = store->keys; !IS_LAST(kv); kv++)
        if (kv->key == key)
            return kv;
    return NULL;
}

void kv_set(uint32_t key, uint32_t val) {
    kv_t *ex = lookup(key);
    if (ex) {
        if (ex->value == val)
            return;
        uint32_t z = 0;
        flash_program(&ex->key, &z, 4);
    }
    kv_t data = {key, val};
    uint32_t idx = num_entries();
    flash_program(&store->keys[idx], &data, sizeof(data));
    if (idx >= NUM_KV - 1)
        kv_gc();
}

bool kv_has(uint32_t key) {
    return lookup(key) != NULL;
}

uint32_t kv_get_defl(uint32_t key, uint32_t defl) {
    kv_t *v = lookup(key);
    if (v)
        return v->value;
    else
        return defl;
}

uint32_t kv_get(uint32_t key) {
    return kv_get_defl(key, 0);
}
