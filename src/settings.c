/*
Settings storage:

disabled/enabled services


light:
apa vs neopixel
strip length
generally, default register config?

*/

#include "jdsimple.h"

#define MAGIC 0x7ab377bb

typedef struct {
    uint16_t reg;
    uint16_t service_low;
    int32_t value;
} key_value_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    char name[32];
    key_value_t keys[];
} settings_t;

#define settings ((settings_t *)SETTINGS_START)

#ifndef DEFAULT_NAME
#define DEFAULT_NAME "jdm"
#endif

void settings_init() {
    if (settings->magic != MAGIC || settings->version != 0) {
        flash_erase(settings);
        settings_t init = {.magic = MAGIC, .version = 0, .name = DEFAULT_NAME};
        flash_program(settings, &init, sizeof(init));
        if (settings->magic != MAGIC)
            jd_panic();
    }
}

static void settings_gc(void) {
    target_disable_irq();
    
    target_reset();
}