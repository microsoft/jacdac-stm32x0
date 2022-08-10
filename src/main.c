#include "lib.h"
#include "services/interfaces/jd_hw_pwr.h"

#ifndef JD_SKIP_MAIN

uint32_t now;

static const uint8_t output_pins[] = {
    PIN_LOG0, PIN_LOG1, PIN_LOG2,    PIN_LOG3, // JD-impl logging
    PIN_P0,   PIN_P1,                          // application logging
    PIN_LED,  PIN_LED2, PIN_LED_GND,           // LED
};

void led_init(void) {
    // To save power, especially in STOP mode,
    // configure all pins in GPIOA,B,C as analog inputs (except for SWD)
#ifdef JD_USE_LSE
#define LAST_PIN PC_13
#else
#define LAST_PIN PC_15
#endif
    for (unsigned i = 0; i <= LAST_PIN; ++i)
        if (i != 13 && i != 14) // 13/14 are SWD pins
            pin_setup_analog_input(i);

#ifdef GPIOF
    // also do all GPIOF (note that it's not enough to just clear PF0 and PF1
    // - the other ones seem to still draw power in stop mode)
    for (unsigned i = 0; i < 16; ++i)
        pin_setup_analog_input(i + 0x50);
#endif

        // The effects of the pins shutdown above is quite dramatic - without the MCU can draw
        // ~100uA (but with wide random variation) in STOP; with shutdown we get a stable 4.3uA

#ifdef DISABLE_SWCLK_FUNC
    pin_setup_analog_input(PA_14);
#endif

#ifdef DISABLE_SWD_FUNC
    pin_setup_analog_input(PA_13);
#endif

    // setup all our output pins
    for (unsigned i = 0; i < sizeof(output_pins); ++i)
        pin_setup_output(output_pins[i]);

    pin_set(PIN_LED_GND, 0);
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

// AP2112
// reg 85ua
// reg back 79uA
// no reg 30uA

static void do_nothing(void) {}
void sleep_forever(void) {
    target_wait_us(500000);
    int cnt = 0;
    for (;;) {
        pin_pulse(PIN_P0, 2);
        tim_set_timer(10000, do_nothing);
        if (cnt++ > 30) {
            cnt = 0;
            adc_read_pin(PIN_LED_GND);
            adc_read_temp();
        }
        pwr_sleep();
        // rtc_deepsleep();
        // rtc_set_to_seconds_and_standby();
    }
}

int main(void) {
    led_init();

    if ((jd_device_id() + 1) == 0)
        target_reset();

    tim_init();
    adc_init_random(); // 300b
    rtc_init();
    uart_init_();

    jd_init();

#if JD_USB_BRIDGE
    usbserial_init();
#endif

    // When BMP attaches, and we're in deep sleep mode, it will scan us as generic Cortex-M0.
    // The flashing scripts scans once, resets the target (using NVIC), and scans again.
    // The delay is so that the second scan detects us as the right kind of chip.
    // TODO this may no longer be the case with latest BMP
    uint32_t startup_wait = tim_get_micros() + 300000;

    while (1) {
        jd_process_everything();

        if (startup_wait) {
            if (in_future(startup_wait))
                continue; // no sleep
            else
                startup_wait = 0;
        }

        pwr_sleep();
    }
}

static void led_panic_blink(void) {
#ifdef PIN_LED_R
    // TODO should we actually PWM?
    pin_setup_output(PIN_LED_R);
    // it doesn't actually matter if LED_RGB_COMMON_CATHODE is defined, as we're just blinking
    pin_set(PIN_LED_R, 0);
    target_wait_us(70000);
    pin_set(PIN_LED_R, 1);
    target_wait_us(70000);
#else
    pin_setup_output(PIN_LED);
    pin_setup_output(PIN_LED_GND);
    pin_set(PIN_LED_GND, 0);
    pin_set(PIN_LED, 1);
    target_wait_us(70000);
    pin_set(PIN_LED, 0);
    target_wait_us(70000);
#endif
}

void hw_panic(void) {
    DMESG("PANIC!");
    target_disable_irq();
    for (int i = 0; i < 60; ++i) {
        led_panic_blink();
    }
    target_reset();
}

#endif