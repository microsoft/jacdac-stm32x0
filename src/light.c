#include "jdsimple.h"

#define LIGHT_TYPE_WS2812B_GRB 0
#define LIGHT_TYPE_SK9822 1

#define DEFAULT_INTENSITY 15
#define DEFAULT_NUMPIXELS 14
#define DEFAULT_MAXPOWER 200

#define FRAME_TIME 50000

#define LOG DMESG

#define LIGHT_REG_LIGHTTYPE 0x80
#define LIGHT_REG_NUMPIXELS 0x81
#define LIGHT_REG_DURATION 0x82
#define LIGHT_REG_COLOR 0x83

#define LIGHT_CMD_START_ANIMATION 0x80

struct light_state {
    uint8_t intensity;
    uint8_t lighttype;
    uint16_t numpixels;
    uint16_t maxpower;
    uint16_t duration;
    uint32_t color;
};

REG_DEFINITION(                   //
    light_regs,                   //
    REG_U8(JD_REG_INTENSITY),     //
    REG_U8(LIGHT_REG_LIGHTTYPE),  //
    REG_U16(LIGHT_REG_NUMPIXELS), //
    REG_U16(JD_REG_MAX_POWER),    //
    REG_U16(LIGHT_REG_DURATION),  //
    REG_U32(LIGHT_REG_COLOR),     //
)

static struct light_state state;
static uint32_t *pxbuffer;
static uint16_t pxbuffer_allocated;
static uint32_t nextFrame;
static uint8_t in_tx, inited, service_number;

static uint32_t anim_step, anim_value;
static uint8_t anim_flag;
static cb_t anim_fn;
static uint32_t anim_end;

void light_init(uint8_t service_num) {
    service_number = service_num;
    state.intensity = DEFAULT_INTENSITY;
    state.numpixels = DEFAULT_NUMPIXELS;
    state.maxpower = DEFAULT_MAXPOWER;
}

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

static const uint8_t b_m16[] = {0, 49, 49, 41, 90, 27, 117, 10};
static int isin(uint8_t theta) {
    // reference: based on FASTLed's sin approximation method:
    // [https://github.com/FastLED/FastLED](MIT)
    int offset = theta;
    if (theta & 0x40) {
        offset = 255 - offset;
    }
    offset &= 0x3F; // 0..63

    int secoffset = offset & 0x0F; // 0..15
    if (theta & 0x40)
        secoffset++;

    int section = offset >> 4; // 0..3
    int s2 = section * 2;

    int b = b_m16[s2];
    int m16 = b_m16[s2 + 1];
    int mx = (m16 * secoffset) >> 4;

    int y = mx + b;
    if (theta & 0x80)
        y = -y;

    y += 128;

    return y;
}

static bool is_enabled() {
    return state.numpixels > 0 && state.intensity > 0;
}

static void show();
static void tx_done() {
    if (in_tx == 2) {
        in_tx = 0;
        show();
    } else {
        in_tx = 0;
    }
}

static void set(uint32_t index, uint32_t color) {
    px_set(pxbuffer, index, state.intensity, color);
}

static void show() {
    target_disable_irq();
    if (in_tx) {
        in_tx = 2;
    } else {
        in_tx = 1;
        px_tx(pxbuffer, PX_WORDS(state.numpixels) << 2, tx_done);
    }
    target_enable_irq();
}

static void set_all(uint32_t color) {
    for (int i = 0; i < state.numpixels; ++i)
        set(i, color);
}

static void anim_set_all() {
    anim_fn = NULL;
    set_all(state.color);
    show();
}

static void anim_start(cb_t fn, uint32_t duration) {
    anim_fn = fn;
    if (duration == 0)
        anim_end = 0;
    else
        anim_end = now + duration * 1000;
}
static void anim_finished() {
    if (anim_end == 0)
        anim_fn = NULL;
}

static void anim_frame() {
    if (anim_fn) {
        anim_fn();
        show();
        if (anim_end && in_past(anim_end)) {
            anim_fn = NULL;
        }
    }
}

// ---------------------------------------------------------------------------------------
// rainbow
// ---------------------------------------------------------------------------------------

static void rainbow_step() {
    for (int i = 0; i < state.numpixels; ++i)
        set(i, hsv(((unsigned)(i * 256) / (state.numpixels - 1) + anim_value) & 0xff, 0xff, 0xff));
    anim_value += anim_step;
    if (anim_value >= 0xff) {
        anim_value = 0;
        anim_finished();
    }
}
static void anim_rainbow() {
    anim_value = 0;
    anim_step = 128U / state.numpixels + 1;
    anim_start(rainbow_step, state.duration);
}

// ---------------------------------------------------------------------------------------
// running lights
// ---------------------------------------------------------------------------------------

static void running_lights_step() {
    if (anim_value >= state.numpixels * 2) {
        anim_value = 0;
        anim_finished();
        return;
    }

    anim_value++;
    for (int i = 0; i < state.numpixels; ++i) {
        int level = (state.intensity * ((isin(i + anim_value) * 127) + 128)) >> 8;
        px_set(pxbuffer, i, level, state.color);
    }
}

static void anim_running_lights() {
    anim_value = 0;
    if (!state.color)
        state.color = 0xff0000;
    anim_start(running_lights_step, state.duration);
}

// ---------------------------------------------------------------------------------------
// sparkle
// ---------------------------------------------------------------------------------------

static void sparkle_step() {
    if (anim_value == 0)
        set_all(0);
    anim_value++;

    if (anim_step < 0) {
        anim_step = random_int(state.numpixels - 1);
        set(anim_step, state.color);
    } else {
        set(anim_step, 0);
        anim_step = -1;
    }

    if (anim_value > 50) {
        anim_finished();
        anim_value = 0;
    }
}

static void anim_sparkle() {
    anim_value = 0;
    anim_step = -1;
    if (!state.color)
        state.color = 0xffffff;
    anim_start(sparkle_step, state.duration);
}

// ---------------------------------------------------------------------------------------
// color wipe
// ---------------------------------------------------------------------------------------

static void color_wipe_step() {
    if (anim_value < state.numpixels) {
        set(anim_value, anim_flag ? state.color : 0);
        anim_value++;
    } else {
        anim_flag = !anim_flag;
        anim_value = 0;
        if (anim_flag)
            anim_finished();
    }
}

static void anim_color_wipe() {
    anim_value = 0;
    anim_flag = 1;
    if (!state.color)
        state.color = 0x0000ff;
    anim_start(color_wipe_step, state.duration);
}

// ---------------------------------------------------------------------------------------
// theatre chase
// ---------------------------------------------------------------------------------------
static void theatre_chase_step() {
    if (anim_value < 10) {
        if (anim_step < 3) {
            for (int i = 0; i < state.numpixels; i += 3)
                set(i + anim_step, anim_flag ? state.color : 0);
            anim_flag = !anim_flag;
            anim_step++;
        } else {
            anim_step = 0;
        }
        anim_value++;
    } else {
        anim_value = 0;
        anim_finished();
    }
}

static void anim_theatre_chase() {
    anim_value = 0;
    anim_step = 0;
    anim_flag = 0;
    if (!state.color)
        state.color = 0xff0000;
    anim_start(theatre_chase_step, state.duration);
}

// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------

static cb_t animations[] = {
    NULL,
    anim_set_all,
    anim_rainbow,
    anim_running_lights,
    anim_color_wipe,
    NULL, // comet
    anim_theatre_chase,
    anim_sparkle,
};

void light_process() {
    // we always check timer to avoid problem with timer overflows
    bool should = should_sample(&nextFrame, FRAME_TIME);

    if (!is_enabled())
        return;

    if (should)
        anim_frame();
}

static void sync_config() {
    if (!is_enabled()) {
        // PIN_PWR has reverse polarity
        pin_set(PIN_PWR, 1);
        return;
    }

    if (!inited) {
        inited = true;
        px_init();
    }

    int needed = PX_WORDS(state.numpixels);
    if (needed > pxbuffer_allocated) {
        pxbuffer_allocated = needed;
        pxbuffer = alloc(needed * 4);
    }

    pin_set(PIN_PWR, 0);
}

static void start_animation(jd_packet_t *pkt) {
    if (pkt->service_size == 0)
        return;
    int anim = pkt->data[0];
    if (anim < sizeof(animations) / sizeof(animations[0])) {
        cb_t f = animations[anim];
        if (f) {
            anim_step = 0;
            anim_flag = 0;
            anim_value = 0;
            LOG("start anim %d", anim);
            f();
        }
    }
}

void light_handle_packet(jd_packet_t *pkt) {
    switch (pkt->service_command) {
    case LIGHT_CMD_START_ANIMATION:
        sync_config();
        start_animation(pkt);
        break;
    default:
        handle_reg(&state, pkt, light_regs);
        break;
    }
}

const host_service_t host_light = {
    .service_class = JD_SERVICE_CLASS_LIGHT,
    .init = light_init,
    .process = light_process,
    .handle_pkt = light_handle_packet,
};
