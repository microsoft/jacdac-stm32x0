#include "jdsimple.h"

uint32_t now;

static const uint8_t output_pins[] = {
    PIN_LOG0, PIN_LOG1,     PIN_LOG2,    PIN_LOG3,    PIN_LED,    PIN_LED2,    PIN_PWR,
    PIN_P0,   PIN_P1,       PIN_ASCK,    PIN_AMOSI,   PIN_SERVO,  PIN_LED_GND, PIN_GLO0,
    PIN_GLO1, PIN_ACC_MOSI, PIN_ACC_SCK, PIN_ACC_VCC, PIN_ACC_CS,
};

void led_init() {
    for (unsigned i = 0; i < sizeof(output_pins); ++i)
        pin_setup_output(output_pins[i]);

    // all power pins are reverse polarity
    pin_set(PIN_PWR, 1);
    pin_set(PIN_GLO0, 1);
    pin_set(PIN_GLO1, 1);
}

void led_toggle() {
    pin_toggle(PIN_LED);
}

void led_set(int state) {
    pin_set(PIN_LED, state);
}

static uint64_t led_off_time;

void led_blink(int us) {
    led_off_time = tim_get_micros() + us;
    led_set(1);
}

int main(void) {
#if 0
    led_init();
    led_set(1);
    tim_init();
    led_blink(200000); // initial (on reset) blink

    while (1) {
        uint64_t now_long = tim_get_micros();
        now = (uint32_t)now_long;

        if (led_off_time) {
            int timeLeft = led_off_time - now_long;
            if (timeLeft <= 0) {
                led_off_time = 0;
                led_set(0);
            } else if (timeLeft < 1000) {
                continue; // don't sleep
            }
        }
    }
    #endif
}

void jd_panic(void) {
    DMESG("PANIC!");
    target_disable_irq();
    while (1) {
        led_toggle();
        target_wait_us(70000);
    }
}

void fail_and_reset() {
    DMESG("FAIL!");
    target_disable_irq();
    for (int i = 0; i < 20; ++i) {
        led_toggle();
        target_wait_us(70000);
    }
    target_reset();
}
