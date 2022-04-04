#include "lib.h"

// faster versions of memcpy() and memset()
void *memcpy(void *dst, const void *src, size_t sz) {
    void *dst0 = dst;
    if (sz >= 4 && !((uintptr_t)dst & 3) && !((uintptr_t)src & 3)) {
        size_t cnt = sz >> 2;
        volatile uint32_t *d = (uint32_t *)dst;
        const uint32_t *s = (const uint32_t *)src;
        while (cnt--) {
            *d++ = *s++;
        }
        sz &= 3;
        dst = (void *)d;
        src = s;
    }

    // see comment in memset() below (have not seen optimization here, but better safe than sorry)
    volatile uint8_t *dd = (uint8_t *)dst;
    volatile uint8_t *ss = (uint8_t *)src;

    while (sz--) {
        *dd++ = *ss++;
    }

    return dst0;
}

void *memset(void *dst, int v, size_t sz) {
    void *dst0 = dst;
    if (sz >= 4 && !((uintptr_t)dst & 3)) {
        size_t cnt = sz >> 2;
        uint32_t vv = 0x01010101 * v;
        uint32_t *d = (uint32_t *)dst;
        while (cnt--) {
            *d++ = vv;
        }
        sz &= 3;
        dst = d;
    }

    // without volatile here, GCC may optimize the loop to memset() call which is obviously not
    // great
    volatile uint8_t *dd = (uint8_t *)dst;

    while (sz--) {
        *dd++ = v;
    }

    return dst0;
}

uint32_t random_int(uint32_t max) {
    if (max == 0)
        return 0;
    uint32_t mask = 0x1;
    while (mask < max)
        mask = (mask << 1) | 1;
    for (;;) {
        uint32_t v = jd_random() & mask;
        if (v <= max)
            return v;
    }
}
