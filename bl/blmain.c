#include "bl.h"

ctx_t ctx_;

static const uint8_t output_pins[] = {
    PIN_LOG0, PIN_LOG1,     PIN_LOG2,    PIN_LOG3,    PIN_LED,    PIN_LED2,    PIN_PWR,
    PIN_P0,   PIN_P1,       PIN_ASCK,    PIN_AMOSI,   PIN_SERVO,  PIN_LED_GND, PIN_GLO0,
    PIN_GLO1, PIN_ACC_MOSI, PIN_ACC_SCK, PIN_ACC_VCC, PIN_ACC_CS,
};

void led_init() {
    for (unsigned i = 0; i < sizeof(output_pins); ++i) {
        pin_setup_output(output_pins[i]);
        // all power pins are reverse polarity; we don't care much for others
        pin_set(output_pins[i], 1);
    }
    pin_set(PIN_LED_GND, 0);
}

void led_set(int state) {
    pin_set(PIN_LED, state);
}

void led_blink(int us) {
    ctx_.led_off_time = tim_get_micros() + us;
    led_set(1);
}

uint32_t hash(const void *data, unsigned len) {
    const uint8_t *d = (const uint8_t *)data;
    uint32_t h = 0x13;
    while (len--)
        h = (h ^ *d++) * 0x1000193;
    return h;
}

uint32_t random(ctx_t *ctx) {
    // xorshift algorithm
    uint32_t x = ctx->randomseed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    ctx->randomseed = x;
    return x;
}

static const uint32_t announce_data[] = {
    8 | (JD_SERVICE_NUMBER_CTRL << 8) | (JD_CMD_ADVERTISEMENT_DATA << 16), //
    JD_SERVICE_CLASS_CTRL,                                                 //
    JD_SERVICE_CLASS_BOOTLOADER                                            //
};

int main(void) {
    __disable_irq();
    clk_setup_pll();
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM17 | LL_APB1_GRP2_PERIPH_USART1);

    ctx_t *ctx = &ctx_;

    led_init();
    led_set(1);
    tim_init();
    uart_init(ctx);

    led_blink(200000); // initial (on reset) blink

    uint8_t *membase = (uint8_t)0x20000000;

    uint32_t r0 = hash(membase + 0, 4096);
    uint32_t r1 = hash(membase + 2048, 2048);
    ctx->randomseed = r0;
    random(ctx); // rotate
    ctx->txBuffer.device_identifier = ((uint64_t)r0 << 32) | r1;
    ctx->service_class_bl = announce_data[2];

    ctx->next_announce = 500000;

    while (1) {
        ctx->now = tim_get_micros();

        jd_process(ctx);

        if (ctx->now >= ctx->next_announce && !ctx->tx_full) {
            memcpy(ctx->txBuffer.data, announce_data, sizeof(announce_data));
            led_blink(200);
            ctx->tx_full = 1;
            ctx->next_announce = ctx->now + 500000;
        }

        if (ctx->led_off_time && ctx->led_off_time < ctx->now) {
            led_set(0);
            ctx->led_off_time = 0;
        }
    }
}

static void busy_sleep(int ms) {
    ms *= 1000; // check
    while (ms--)
        asm volatile("nop");
}

static void led_panic_blink() {
    led_set(1);
    busy_sleep(70);
    led_set(0);
    busy_sleep(70);
}

void jd_panic(void) {
    DMESG("PANIC!");
    while (1) {
        led_panic_blink();
    }
}

void fail_and_reset() {
    DMESG("FAIL!");
    for (int i = 0; i < 10; ++i) {
        led_panic_blink();
    }
    target_reset();
}
