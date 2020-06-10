#include "jdsimple.h"

#define LIGHT_TYPE_WS2812B_GRB 0
#define LIGHT_TYPE_SK9822 1

#define DEFAULT_INTENSITY 15
#define DEFAULT_NUMPIXELS 15
#define DEFAULT_MAXPOWER 200

#define LOG DMESG

#define LIGHT_REG_LIGHTTYPE 0x80
#define LIGHT_REG_NUMPIXELS 0x81
#define LIGHT_REG_DURATION 0x82
#define LIGHT_REG_COLOR 0x83

#define LIGHT_CMD_RUN 0x81

#define LIGHT_PROG_SET 0xD0         // P, R, C+
#define LIGHT_PROG_FADE 0xD1        // P, N, C+
#define LIGHT_PROG_FADE_HSV 0xD2    // P, N, C+
#define LIGHT_PROG_ROTATE_FWD 0xD3  // P, N, K
#define LIGHT_PROG_ROTATE_BACK 0xD4 // P, N, K
#define LIGHT_PROG_WAIT 0xD5        // K

#define LIGHT_PROG_COLN 0xC0
#define LIGHT_PROG_COL1 0xC1
#define LIGHT_PROG_COL2 0xC2
#define LIGHT_PROG_COL3 0xC3

#define PROG_EOF 0
#define PROG_CMD 1
#define PROG_NUMBER 3
#define PROG_COLOR_BLOCK 4

REG_DEFINITION(                   //
    light_regs,                   //
    REG_SRV_BASE,                 //
    REG_U8(JD_REG_INTENSITY),     //
    REG_U8(LIGHT_REG_LIGHTTYPE),  //
    REG_U16(LIGHT_REG_NUMPIXELS), //
    REG_U16(JD_REG_MAX_POWER),    //
    REG_U16(LIGHT_REG_DURATION),  //
    REG_U32(LIGHT_REG_COLOR),     //
)

struct srv_state {
    SRV_COMMON;

    uint8_t intensity;
    uint8_t lighttype;
    uint16_t numpixels;
    uint16_t maxpower;
    uint16_t duration;
    uint32_t color;

    uint32_t *pxbuffer;
    uint16_t pxbuffer_allocated;
    volatile uint8_t in_tx;
    volatile uint8_t dirty;
    uint8_t inited;

    uint8_t prog_ptr;
    uint8_t prog_size;
    uint32_t prog_next_step;
    uint8_t prog_data[JD_SERIAL_PAYLOAD_SIZE + 1];

    uint8_t anim_flag;
    uint32_t anim_step, anim_value;
    srv_cb_t anim_fn;
    uint32_t anim_end;
};

static srv_t *state_;

static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 16) | (g << 8) | b;
}

static uint32_t hsv(uint8_t hue, uint8_t sat, uint8_t val) {
    // scale down to 0..192
    hue = (hue * 192) >> 8;

    // reference: based on FastLED's hsv2rgb rainbow algorithm
    // [https://github.com/FastLED/FastLED](MIT)
    int invsat = 255 - sat;
    int brightness_floor = (val * invsat) >> 8;
    int color_amplitude = val - brightness_floor;
    int section = (hue / 0x40) >> 0; // [0..2]
    int offset = (hue % 0x40) >> 0;  // [0..63]

    int rampup = offset;
    int rampdown = (0x40 - 1) - offset;

    int rampup_amp_adj = ((rampup * color_amplitude) / (256 / 4));
    int rampdown_amp_adj = ((rampdown * color_amplitude) / (256 / 4));

    int rampup_adj_with_floor = (rampup_amp_adj + brightness_floor);
    int rampdown_adj_with_floor = (rampdown_amp_adj + brightness_floor);

    int r, g, b;
    if (section) {
        if (section == 1) {
            // section 1: 0x40..0x7F
            r = brightness_floor;
            g = rampdown_adj_with_floor;
            b = rampup_adj_with_floor;
        } else {
            // section 2; 0x80..0xBF
            r = rampup_adj_with_floor;
            g = brightness_floor;
            b = rampdown_adj_with_floor;
        }
    } else {
        // section 0: 0x00..0x3F
        r = rampdown_adj_with_floor;
        g = rampup_adj_with_floor;
        b = brightness_floor;
    }
    return rgb(r, g, b);
}

static bool is_empty(const uint32_t *data, uint32_t size) {
    for (unsigned i = 0; i < size; ++i) {
        if (data[i])
            return false;
    }
    return true;
}

static bool is_enabled(srv_t *state) {
    // TODO check if all pixels are zero
    return state->numpixels > 0 && state->intensity > 0;
}

static void set(srv_t *state, uint32_t index, uint32_t color) {
    px_set(state->pxbuffer, index, color);
}

static void set_safe(srv_t *state, uint32_t index, uint32_t color) {
    if (index < state->numpixels)
        px_set(state->pxbuffer, index, color);
}

static void show(srv_t *state) {
    state->dirty = 1;
}

#define SCALE0(c, i) ((((c)&0xff) * (1 + (i & 0xff))) >> 8)
#define SCALE(c, i)                                                                                \
    (SCALE0((c) >> 0, i) << 0) | (SCALE0((c) >> 8, i) << 8) | (SCALE0((c) >> 16, i) << 16)

static void tx_done(void) {
    pwr_leave_pll();
    state_->in_tx = 0;
}

static void limit_intensity(srv_t *state) {
    uint8_t *d = (uint8_t *)state->pxbuffer;
    unsigned n = state->numpixels * 3;

    int current_full = 0;
    int current = 0;
    while (n--) {
        uint8_t v = *d++;
        current += SCALE0(v, state->intensity) * 46;
        current_full += v * 46;
    }

    // 14mA is the chip at 48MHz, 930uA per LED is static
    int base_current = 14000 + 930 * state->numpixels;
    int current_limit = state->maxpower * 1000 - base_current;

    if (current <= current_limit) {
        // DMESG("curr: %dmA; not limiting %d", (base_current + current) / 1000, state->intensity);
        return;
    }

    int inten = current_limit / (current_full >> 8) - 1;
    if (inten < 0)
        inten = 0;
    DMESG("limiting %d -> %d; %dmA", state->intensity, inten,
          (base_current + (current_full * inten >> 8)) / 1000);
    state->intensity = inten;
}

static uint32_t prog_fetch_color(srv_t *state) {
    uint32_t ptr = state->prog_ptr;
    if (ptr + 3 > state->prog_size)
        return 0;
    uint8_t *d = state->prog_data + ptr;
    state->prog_ptr = ptr + 3;
    return (d[0] << 0) | (d[1] << 8) | (d[2] << 16);
}

static int prog_fetch(srv_t *state, uint32_t *dst) {
    if (state->prog_ptr >= state->prog_size)
        return PROG_EOF;
    uint8_t *d = state->prog_data;
    uint8_t c = d[state->prog_ptr++];
    if (!(c & 0x80)) {
        *dst = c;
        return PROG_NUMBER;
    } else if ((c & 0xc0) == 0x80) {
        *dst = ((c & 0x3f) << 8) | d[state->prog_ptr++];
        return PROG_NUMBER;
    } else if ((c & 0xf0) == 0xd0) {
        *dst = c;
        return PROG_CMD;
    } else
        switch (c) {
        case LIGHT_PROG_COL1:
            *dst = 1;
            return PROG_COLOR_BLOCK;
        case LIGHT_PROG_COL2:
            *dst = 2;
            return PROG_COLOR_BLOCK;
        case LIGHT_PROG_COL3:
            *dst = 3;
            return PROG_COLOR_BLOCK;
        case LIGHT_PROG_COLN:
            *dst = d[state->prog_ptr++];
            return PROG_COLOR_BLOCK;
        default:
            *dst = c;
            return PROG_CMD;
        }
}

static int prog_fetch_num(srv_t *state, int defl) {
    uint8_t prev = state->prog_ptr;
    uint32_t res;
    int r = prog_fetch(state, &res);
    if (r == PROG_NUMBER)
        return res;
    else {
        state->prog_ptr = prev; // rollback
        return defl;
    }
}

static int prog_fetch_cmd(srv_t *state) {
    uint32_t cmd;
    for (;;) {
        switch (prog_fetch(state, &cmd)) {
        case PROG_CMD:
            return cmd;
        case PROG_COLOR_BLOCK:
            while (cmd--)
                prog_fetch_color(state);
            break;
        case PROG_EOF:
            return 0;
        }
    }
}

static void prog_set(srv_t *state, uint32_t idx, uint32_t rep, uint32_t len) {
    uint8_t start = state->prog_ptr;
    uint32_t maxrep = (state->numpixels / len) + 1;
    if (rep > maxrep)
        rep = maxrep;
    while (rep--) {
        state->prog_ptr = start;
        for (uint32_t i = 0; i < len; ++i) {
            set_safe(state, idx++, prog_fetch_color(state));
        }
    }
}

static void prog_fade(srv_t *state, uint32_t idx, uint32_t rep, uint32_t len, bool usehsv) {
    if (len < 2) {
        prog_set(state, idx, rep, len);
        return;
    }
    unsigned colidx = 0;
    uint32_t col0 = prog_fetch_color(state);
    uint32_t col1 = prog_fetch_color(state);

    uint32_t colstep = ((len - 1) << 16) / rep;
    uint32_t colpos = 0;
    uint32_t end = idx + rep;
    if (end >= state->numpixels)
        end = state->numpixels;
    while (idx < end) {
        while (colidx < (colpos >> 16)) {
            colidx++;
            col0 = col1;
            col1 = prog_fetch_color(state);
        }
        uint32_t fade1 = colpos & 0xffff;
        uint32_t fade0 = 0xffff - fade1;

#define MIX(sh)                                                                                    \
    (((((col0 >> sh) & 0xff) * fade0 + ((col1 >> sh) & 0xff) * fade1 + 0x8000) >> 16) << sh)

        uint32_t col = MIX(0) | MIX(8) | MIX(16);
        set(state, idx++, usehsv ? hsv(col & 0xff, (col >> 8) & 0xff, col >> 16) : col);
        colpos += colstep;
    }
}

#define AT(idx) ((uint8_t *)state->pxbuffer)[(idx)*3]

static void prog_rot(srv_t *state, uint32_t idx, uint32_t len, uint32_t shift) {
    uint32_t end = idx + len;
    if (end > state->numpixels)
        end = state->numpixels;
    if (shift == 0 || end <= idx)
        return;

    uint8_t *first = &AT(idx);
    uint8_t *middle = &AT(idx + shift);
    uint8_t *last = &AT(end);
    uint8_t *next = middle;

    while (first != next) {
        uint8_t tmp = *first;
        *first++ = *next;
        *next++ = tmp;

        if (next == last)
            next = middle;
        else if (first == middle)
            middle = next;
    }
}

static void prog_process(srv_t *state) {
    if (state->prog_ptr >= state->prog_size)
        return;
    if (in_future(state->prog_next_step))
        return;

    for (;;) {
        int cmd = prog_fetch_cmd(state);
        DMESG("cmd:%x", cmd);
        if (!cmd)
            break;

        if (cmd == LIGHT_PROG_WAIT) {
            uint32_t k = prog_fetch_num(state, 50);
            state->prog_next_step = now + k * 1000;
            break;
        }

        switch (cmd) {
        case LIGHT_PROG_FADE:
        case LIGHT_PROG_FADE_HSV:
        case LIGHT_PROG_SET: {
            int idx = prog_fetch_num(state, 0);
            int rep = prog_fetch_num(state, cmd == LIGHT_PROG_SET ? 1 : state->numpixels - idx);
            uint32_t len;
            if (rep <= 0 || prog_fetch(state, &len) != PROG_COLOR_BLOCK || len == 0)
                continue; // bailout
            DMESG("%x %d %d l=%d", cmd, idx, rep, len);
            if (cmd == LIGHT_PROG_SET)
                prog_set(state, idx, rep, len);
            else
                prog_fade(state, idx, rep, len, cmd == LIGHT_PROG_FADE_HSV);
            break;
        }

        case LIGHT_PROG_ROTATE_BACK:
        case LIGHT_PROG_ROTATE_FWD: {
            int idx = prog_fetch_num(state, -1);
            int len = prog_fetch_num(state, -1);
            int k = prog_fetch_num(state, -1);
            if (k == -1) {
                k = len;
                len = -1;
                if (k == -1) {
                    k = idx;
                    idx = -1;
                    if (k == -1)
                        k = 1;
                }
            }
            if (idx < 0)
                idx = 0;
            if (len < 0)
                len = state->numpixels;
            if (idx >= state->numpixels)
                continue;
            if (idx + len > state->numpixels)
                len = state->numpixels - idx;
            if (len == 0)
                continue;
            while (k >= len)
                k -= len;
            if (cmd == LIGHT_PROG_ROTATE_FWD && k != 0)
                k = len - k;
            DMESG("%x %d %d l=%d", cmd, idx, k, len);
            prog_rot(state, idx, len, k);
            break;
        }
        }
    }

    show(state);
}

void light_process(srv_t *state) {
    prog_process(state);

    if (!is_enabled(state))
        return;

    if (state->dirty && !state->in_tx) {
        state->dirty = 0;
        if (is_empty(state->pxbuffer, PX_WORDS(state->numpixels))) {
            pwr_pin_enable(0);
            return;
        } else {
            pwr_pin_enable(1);
        }
        state->in_tx = 1;
        pwr_enter_pll();
        limit_intensity(state);
        px_tx(state->pxbuffer, state->numpixels * 3, state->intensity, tx_done);
    }
}

static void sync_config(srv_t *state) {
    if (!is_enabled(state)) {
        pwr_pin_enable(0);
        return;
    }

    if (!state->inited) {
        state->inited = true;
        px_init();
    }

    int needed = PX_WORDS(state->numpixels);
    if (needed > state->pxbuffer_allocated) {
        state->pxbuffer_allocated = needed;
        state->pxbuffer = alloc(needed * 4);
    }

    pwr_pin_enable(1);
}

static void handle_run_cmd(srv_t *state, jd_packet_t *pkt) {
    state->prog_size = pkt->service_size;
    state->prog_ptr = 0;
    memcpy(state->prog_data, pkt->data, state->prog_size);
    state->prog_next_step = now;
    sync_config(state);
}

void light_handle_packet(srv_t *state, jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case LIGHT_CMD_RUN:
        handle_run_cmd(state, pkt);
        break;
    default:
        srv_handle_reg(state, pkt, light_regs);
        break;
    }
}

SRV_DEF(light, JD_SERVICE_CLASS_LIGHT);
void light_init() {
    SRV_ALLOC(light);
    state_ = state; // there is global singleton state
    state->intensity = DEFAULT_INTENSITY;
    state->numpixels = DEFAULT_NUMPIXELS;
    state->maxpower = DEFAULT_MAXPOWER;

#if 0
    state->maxpower = 20;

    state->numpixels = 14;
    state->intensity = 0xff;
    state->color = 0x00ff00;
    state->duration = 100;

    sync_config(state);
    start_animation(state, 2);
#endif
}
