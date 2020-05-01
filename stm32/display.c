#include "jdstm.h"

#ifdef NUM_DISPLAY_ROWS

//#define DISP_DELAY_OVERRIDE 300 // for power measurements
#define DISP_LIGHT_SENSE 1

#define DISP_LEVEL_MAX 1800 // regulate brightness here
#define DISP_LEVEL_MIN 1

#define DISP_DELAY_MAX 1300 // assuming 10ms frames, we use 66% of it
#define DISP_DELAY_MIN 20

#define DISP_STEP (((DISP_DELAY_MAX - DISP_DELAY_MIN) << 10) / (DISP_LEVEL_MAX - DISP_LEVEL_MIN))

#define SET(v) (v)
#define CLR(v) ((v) << 16)

typedef struct ctx {
    GPIO_TypeDef *row_port;
    GPIO_TypeDef *col_port;

    uint16_t row_all;
    uint16_t col_all;

    uint16_t row_mask[NUM_DISPLAY_ROWS];
    uint16_t col_mask[NUM_DISPLAY_COLS];

    uint16_t target_delay;
    uint16_t delay;
    uint8_t num_cached;

    uint16_t frames_until_reading;
    uint16_t reading;

    uint32_t cached_cols[NUM_DISPLAY_ROWS];
    uint32_t cached_rows[NUM_DISPLAY_ROWS];

    uint8_t img[NUM_DISPLAY_COLS];
} ctx_t;

static ctx_t ctx_;

// col -|>- row
static const uint8_t row_pins[NUM_DISPLAY_ROWS] = {PIN_DISPLAY_ROWS};
static const uint8_t col_pins[NUM_DISPLAY_COLS] = {PIN_DISPLAY_COLS};

static GPIO_TypeDef *compute_masks(int n, uint16_t *masks, const uint8_t *pins, uint16_t *all) {
    *all = 0;
    GPIO_TypeDef *port = PIN_PORT(pins[0]);
    for (int i = 0; i < n; ++i) {
        if (PIN_PORT(pins[i]) != port)
            jd_panic();
        masks[i] = PIN_MASK(pins[i]);
        *all |= masks[i];
        pin_setup_output(pins[i]);
    }
    return port;
}

static void set_masks(ctx_t *ctx, uint32_t row, uint32_t col) {
    ctx->row_port->BSRR = row;
    ctx->col_port->BSRR = col;
}

static void turn_off(ctx_t *ctx) {
    set_masks(ctx, SET(ctx->row_all), CLR(ctx->col_all));
}

static void measure_light(ctx_t *ctx) {
    turn_off(ctx);

#ifdef DISP_LIGHT_SENSE
    for (int i = 0; i < NUM_DISPLAY_COLS; ++i) {
        pin_set(col_pins[i], 1);
        pin_setup_analog_input(col_pins[i]);
    }

    uint8_t pin = col_pins[0];
    pin_set(pin, 0);
    pin_setup_output(pin);
    pin_setup_analog_input(pin);
    target_wait_us(2000);
    ctx->reading = adc_read_pin(pin);

    int v = ctx->reading;

    if (v > DISP_LEVEL_MAX)
        v = DISP_LEVEL_MAX;
    else if (v < DISP_LEVEL_MIN)
        v = DISP_LEVEL_MIN;

    v -= DISP_LEVEL_MIN;
    ctx->target_delay = DISP_DELAY_MIN + (v * DISP_STEP >> 10);
#else
    ctx->target_delay = DISP_DELAY_OVERRIDE;
#endif

#ifdef DISP_DELAY_OVERRIDE
    ctx->target_delay = DISP_DELAY_OVERRIDE;
#endif

    // ctx->delay = 100;

    for (int i = 0; i < NUM_DISPLAY_COLS; ++i) {
        pin_set(col_pins[i], 0);
        pin_setup_output(col_pins[i]);
    }
    turn_off(ctx);

    // DMESG("r:%d -> %d", ctx->reading, ctx->delay);
}

static void disp_init(ctx_t *ctx) {
    if (ctx->row_port)
        return;
    ctx->delay = DISP_DELAY_MIN;
    ctx->row_port = compute_masks(NUM_DISPLAY_ROWS, ctx->row_mask, row_pins, &ctx->row_all);
    ctx->col_port = compute_masks(NUM_DISPLAY_COLS, ctx->col_mask, col_pins, &ctx->col_all);
    measure_light(ctx);
    ctx->delay = ctx->target_delay;
}

static void recompute_cache(ctx_t *ctx) {
    int dst = 0;
    for (int i = 0; i < NUM_DISPLAY_ROWS; ++i) {
        uint16_t bits = 0;
        for (int j = 0; j < NUM_DISPLAY_COLS; ++j)
            if (ctx->img[j] & (1 << (NUM_DISPLAY_ROWS - i - 1)))
                bits |= ctx->col_mask[j];

        if (bits == 0)
            continue;

        ctx->cached_cols[dst] = SET(bits) | CLR(ctx->col_all & ~bits);
        ctx->cached_rows[dst] = SET(ctx->row_all & ~ctx->row_mask[i]) | CLR(ctx->row_mask[i]);
        dst++;
    }
    ctx->num_cached = dst;
}

void disp_show(uint8_t *img) {
    ctx_t *ctx = &ctx_;

    disp_init(ctx);
    if (memcmp(img, ctx->img, NUM_DISPLAY_COLS) == 0)
        return;
    memcpy(ctx->img, img, NUM_DISPLAY_COLS);
    recompute_cache(ctx);
}

int disp_light_level() {
    return ctx_.reading;
}

#ifdef DISP_USE_TIMER
static void do_nothing(void) {}
#endif

void disp_refresh() {
    ctx_t *ctx = &ctx_;

    int dd = ctx->target_delay - ctx->delay;
    int max = ctx->target_delay >> 6;
    if (max < 1)
        max = 1;
    if (dd < -max)
        dd = -max;
    else if (dd > max)
        dd = max;
    ctx->delay += dd;

    disp_init(ctx);
    for (int i = 0; i < ctx->num_cached; ++i) {
        set_masks(ctx, ctx->cached_rows[i], ctx->cached_cols[i]);
#ifdef DISP_USE_TIMER
        tim_set_timer(ctx->delay, do_nothing);
        __WFI();
#else
        target_wait_us(ctx->delay);
#endif
    }
    turn_off(ctx);

    if (ctx->frames_until_reading-- == 0) {
        ctx->frames_until_reading = 50;
        measure_light(ctx);
    }
}

#endif
