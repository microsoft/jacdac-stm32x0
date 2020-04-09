#include "jdsimple.h"

#define STACK_SIZE 512
#define STACK_BASE ((uint32_t)&_estack)
#define HEAP_BASE ((uint32_t)&_end)
#define HEAP_END (STACK_BASE - STACK_SIZE)

extern uint32_t _end;
extern uint32_t _estack;

static uint32_t *aptr;

static uint16_t maxStack;

void alloc_stack_check() {
    uint32_t *ptr = (uint32_t *)(STACK_BASE - STACK_SIZE);
    while (ptr < (uint32_t *)STACK_BASE) {
        if (*ptr != 0x33333333)
            break;
        ptr++;
    }
    int sz = STACK_BASE - (uint32_t)ptr;
    if (sz > maxStack)
        DMESG("used stack: %d", maxStack = sz);
}

void alloc_init() {
    DMESG("space: %d", HEAP_END - HEAP_BASE);

    aptr = (uint32_t *)HEAP_BASE;
    int p = 0;
    int sz = (uint32_t)&p - HEAP_BASE - 32;
    memset((void *)HEAP_BASE, 0x33, sz);

    alloc_stack_check();
}

void *alloc(uint32_t size) {
    alloc_stack_check();
    size = (size + 3) >> 2;
    void *r = aptr;
    aptr += size;
    if ((uint32_t)aptr > HEAP_END)
        fail_and_reset();
    return r;
}
