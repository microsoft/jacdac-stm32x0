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
}

void led_set(int state) {
    pin_set(PIN_LED, state);
}

void led_blink(int us) {
    ctx_.led_off_time = tim_get_micros() + us;
    led_set(1);
}

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

    while (1) {
        ctx->now = tim_get_micros();

        jd_process(ctx);

        if (ctx->led_off_time && ctx->led_off_time < ctx->now) {
            led_set(0);
            ctx->led_off_time = 0;
        }
    }
}

static void led_panic_blink() {
    led_set(1);
    target_wait_us(70000);
    led_set(0);
    target_wait_us(70000);
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
