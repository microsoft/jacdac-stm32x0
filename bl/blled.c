#include "blutil.h"

// The LED_BL will be run at 10/BL_LED_PERIOD and 30/BL_LED_PERIOD
#ifndef BL_LED_PERIOD
#define BL_LED_PERIOD 300
#endif

// if LED is on SWDIO/SWCLK pin, disable it - otherwise it's hard to re-program the board
#if PIN_LED == PA_13 || PIN_LED == PA_14
#undef PIN_LED
#define PIN_LED NO_PIN
#endif

void led_init(void) {
    led_set_value(0);
#ifdef PIN_BL_LED
    blled_init(BL_LED_PERIOD);
#else
    pin_setup_output(PIN_LED);
    pin_setup_output(PIN_LED_GND);
#if QUICK_LOG == 1
    pin_setup_output(PIN_X0);
    pin_setup_output(PIN_X1);
#endif
    pin_set(PIN_LED_GND, 0);
#endif
#if QUICK_LOG == 1
    pin_setup_output(PIN_X0);
    pin_setup_output(PIN_X1);
#endif
}

void led_set_value(int v) {
#ifdef PIN_BL_LED
#ifdef LED_RGB_COMMON_CATHODE
    blled_set_duty(v);
#else
    blled_set_duty(BL_LED_PERIOD - (v));
#endif
#else
    pin_set(PIN_LED, v != 0);
#endif
}

static void led_panic_blink(void) {
    led_set_value(30);
    target_wait_us(64 << 20);
    led_set_value(0);
    target_wait_us(64 << 20);
}

void hw_panic(void) {
    DMESG("PANIC!");
    while (1) {
        led_panic_blink();
    }
}
