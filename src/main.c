#include "jdsimple.h"

static const uint8_t output_pins[] = {
    PIN_LOG0, PIN_LOG1,     PIN_LOG2,    PIN_LOG3,    PIN_LED,    PIN_LED2,    PIN_PWR,
    PIN_P0,   PIN_P1,       PIN_ASCK,    PIN_AMOSI,   PIN_SERVO,  PIN_LED_GND, PIN_GLO0,
    PIN_GLO1, PIN_ACC_MOSI, PIN_ACC_SCK, PIN_ACC_VCC, PIN_ACC_CS,
};

void led_init() {
    // To save power, especially in STOP mode,
    // configure all pins in GPIOA,B,C as analog inputs (except for SWD)
    for (unsigned i = 0; i < 16 * 3; ++i)
        if (i != 13 && i != 14) // 13/14 are SWD pins
            pin_setup_analog_input(i);

    // also do all GPIOF (note that it's not enough to just clear PF0 and PF1
    // - the other ones seem to still draw power in stop mode)
    for (unsigned i = 0; i < 16; ++i)
        pin_setup_analog_input(i + 0x50);

    // The effects of the pins shutdown above is quite dramatic - without the MCU can draw ~100uA
    // (but with wide random variation) in STOP; with shutdown we get a stable 4.3uA

    // setup all our output pins
    for (unsigned i = 0; i < sizeof(output_pins); ++i)
        pin_setup_output(output_pins[i]);

    // all power pins are reverse polarity
    pin_set(PIN_PWR, 1);
    pin_set(PIN_GLO0, 1);
    pin_set(PIN_GLO1, 1);
}

void log_pin_set(int line, int v) {
#ifdef BUTTON_V0
    switch (line) {
    case 4:
        pin_set(PIN_LOG0, v);
        break;
    case 1:
        pin_set(PIN_LOG1, v);
        break;
    case 2:
        pin_set(PIN_LOG2, v);
        break;
    case 0:
        // pin_set(PIN_LOG3, v);
        break;
    }
#else
// do nothing
#endif
}

static uint64_t led_off_time;

void led_toggle() {
    pin_toggle(PIN_LED);
}

void led_set(int state) {
    pin_set(PIN_LED, state);
}

void led_blink(int us) {
    led_off_time = tim_get_micros() + us;
    led_set(1);
}
int main(void) {
    led_init();
    led_set(1);

    alloc_init();

    tim_init();
    px_init();

    adc_init_random(); // 300b

    jd_init();

#if 0
    target_wait_us(5000000);
    led_set(0);
    LL_PWR_SetPowerMode(LL_PWR_MODE_STOP_LPREGU);
    LL_LPM_EnableDeepSleep();
    __WFI();
#endif

#ifdef LOW_POWER
    rtc_init();
#endif

    app_init_services();

    led_off_time = tim_get_micros() + 200000;

    // we run without sleep at the beginning to allow connecting debugger
    // uint64_t start_time = led_off_time + 3000000;
    uint64_t start_time = led_off_time;

    // rtc_set_led_duty(200);

    while (1) {
        uint64_t now_long = tim_get_micros();
        now = (uint32_t)now_long;

        if (led_off_time && now_long > led_off_time) {
            led_off_time = 0;
            led_set(0);
        }

        if (start_time && now_long > start_time) {
            start_time = 0;
        }

        app_process();

#ifdef LOW_POWER
        if (!led_off_time && !start_time) {
            rtc_sleep();
        }
#endif
    }
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
    NVIC_SystemReset();
}
