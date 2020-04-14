/*
Settings storage:

disabled/enabled services


light:
apa vs neopixel
strip length
generally, default register config?

*/

#include "jdsimple.h"

struct srv_state {
    SRV_COMMON;
};

#define REG_KEY(srv, reg) ((srv->vt->service_class + srv->instance_idx) & 0xffff) | (reg << 16)

void settings_set_reg(srv_t *srv, unsigned reg, int val) {
    CHECK(reg <= 0xfff);
    kv_set(REG_KEY(srv, reg), val);
}

int settings_get_reg(srv_t *srv, unsigned reg) {
    CHECK(reg <= 0xfff);
    return kv_get(REG_KEY(srv, reg));
}

void settings_set_binary(unsigned id, const void *data, unsigned len) {
    CHECK(len < 0xff);
    CHECK(id < 0xff);

    uint32_t reg = (0x8000 | id) << 16;
    kv_set(reg, len);

    const uint8_t *dp = data;
    for (unsigned i = 0; i < len; i += 4) {
        uint32_t val = dp[i] | (dp[i + 1] << 8) | (dp[i + 2] << 16) | (dp[i + 3] << 24);
        kv_set(reg | (1 + (i >> 2)), val);
    }
}

int settings_get_binary(unsigned id, void *data, unsigned maxlen) {
    CHECK((maxlen & 3) == 0);

    // this encoding may leave binary trash on gc, but it doesn't seem it's a big problem

    uint32_t reg = (0x8000 | id) << 16;
    if (!kv_has(reg))
        return -1;
    uint32_t val = kv_get(reg);
    unsigned len = val & 0xff;

    if (maxlen > len)
        maxlen = len;

    uint8_t *dp = data;
    for (int i = 0; i < maxlen; i += 4) {
        val = kv_get(reg | (1 + (i << 2)));
        dp[i] = val;
        dp[i + 1] = val >> 8;
        dp[i + 2] = val >> 16;
        dp[i + 3] = val >> 24;
    }

    return len;
}

void settings_init() {
    kv_init();
}
