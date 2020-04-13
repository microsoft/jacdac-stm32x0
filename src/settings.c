/*
Settings storage:

disabled/enabled services


light:
apa vs neopixel
strip length
generally, default register config?

*/

#include "jdsimple.h"

#define REG_KEY(serv, reg)

void settings_set_string() {}

void settings_init() {
    kv_init();
}
