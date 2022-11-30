#include "lib.h"

// (not used)

// STM32G0/WL require 8-byte aligned flash writes
#if !defined(STM32G0) && !defined(STM32WL)

#define MAGIC 0xe233b285
#define SETTINGS_SIZE 1024
#define SETTINGS_START (0x08000000 + (JD_FLASH_SIZE - BL_SIZE - SETTINGS_SIZE))

extern uint32_t __etext;

typedef struct {
    uint8_t size;
    uint8_t deviceid[8];
    // we need to be able to clear the hash - so it has to be in different half-word than size
    uint8_t hash;
    char name[];
} ns_t;

typedef struct {
    uint32_t magic;
    ns_t keys[];
} store_t;

#define store ((store_t *)SETTINGS_START)

#define NEXT(p) (ns_t *)((uint8_t *)(p) + (p)->size)

static void ns_gc(void) {
    target_disable_irq();

    store_t *tmp = jd_alloc_emergency_area(SETTINGS_SIZE);
    memcpy(tmp, store, SETTINGS_SIZE);

    // first erase, so that we have a clear bailout path
    flash_erase(store);

    ns_t *kv = tmp->keys;
    uint32_t numdel = 0;

    while (kv->size != 0xff) {
        if (kv->hash == 0)
            numdel++;
        kv = NEXT(kv);
    }

    if (numdel <= 1) {
        // whoops, we're out of space even after compression! just bail out (flash already erased)
        target_reset();
    }

    uint8_t *dst = (uint8_t *)tmp->keys;
    uint8_t sz;
    for (kv = tmp->keys; kv->size != 0xff; kv = (ns_t *)((uint8_t *)kv + sz)) {
        sz = kv->size;
        if (kv->hash) {
            if (dst != (uint8_t *)kv)
                memcpy(dst, kv, sz);
            dst = dst + sz;
        }
    }

    flash_program(store, tmp, dst - (uint8_t *)tmp);

    target_reset();
}

void ns_init() {
    if ((uint32_t)&__etext > (uint32_t)store)
        JD_PANIC();
    if (store->magic != MAGIC) {
        flash_erase(store);
        store_t init = {.magic = MAGIC};
        flash_program(store, &init, sizeof(init));
        if (store->magic != MAGIC)
            JD_PANIC();
    }
}

static uint8_t hash(uint64_t key) {
    uint32_t h = (uint32_t)key ^ (uint32_t)(key >> 32);
    h = h ^ (h >> 16);
    h = h ^ (h >> 8);
    if (h == 0xff || h == 0)
        h ^= 1;
    return h;
}

static ns_t *lookup(uint64_t key) {
    uint8_t h = hash(key);
    ns_t *p = store->keys;
    for (;;) {
        uint8_t sz = p->size;
        if (sz == 0xff)
            return p;
        if (p->hash == h) {
            if (memcmp(&key, p->deviceid, 8) == 0)
                return p;
        }
        p = NEXT(p);
    }
}

void ns_clear() {
    uint32_t zero = 0;
    flash_program(store, &zero, 4);
    ns_init();
}

void ns_set(uint64_t key, const char *name) {
    ns_t *ex = lookup(key);
    if (ex->size != 0xff) {
        if (name && strcmp(ex->name, name) == 0)
            return;
        uint16_t z = 0;
        flash_program(&ex->hash - 1, &z, 2);
        ex = lookup(key);
        if (ex->size != 0xff)
            JD_PANIC();
    }
    if (!name)
        return;
    unsigned size = (sizeof(ns_t) + strlen(name) + 2) & ~1;
    if (size > 32)
        JD_PANIC();
    uint8_t buf[size];
    ns_t *tmp = (ns_t *)buf;
    tmp->size = size;
    tmp->hash = hash(key);
    memcpy(tmp->deviceid, &key, 8);
    strcpy(tmp->name, name);
    flash_program(ex, buf, size);
    if ((uint8_t *)ex + size > (uint8_t *)SETTINGS_START + SETTINGS_SIZE - 32)
        ns_gc();
}

const char *ns_get(uint64_t key) {
    ns_t *ex = lookup(key);
    if (ex->size == 0xff)
        return NULL;
    return ex->name;
}

#endif