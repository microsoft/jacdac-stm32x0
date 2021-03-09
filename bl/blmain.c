#include "bl.h"

static void start_app(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
    BL_MAGIC_FLAG = BL_MAGIC_FLAG_APP;
#pragma GCC diagnostic pop
    target_reset();
}

ctx_t ctx_;

void led_init(void) {
    pin_setup_output(PIN_LED);
    pin_setup_output(PIN_LED_GND);
    pin_set(PIN_LED_GND, 0);
}

void led_set(int state) {
    pin_set(PIN_LED, state);
}

void led_blink(int us) {
    ctx_.led_off_time = tim_get_micros() + us;
    led_set(1);
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
    8 | (JD_SERVICE_NUMBER_CONTROL << 8) | (JD_CMD_ANNOUNCE << 16), //
    JD_SERVICE_CLASS_CONTROL,                                       //
    JD_SERVICE_CLASS_BOOTLOADER                                     //
};

uint32_t bl_adc_random_seed(void);

int main(void) {
    __disable_irq();
    clk_setup_pll();

#if USART_IDX == 1
#ifdef STM32G0
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
#else
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP2_PERIPH_USART1);
#endif
#endif
#if USART_IDX == 2
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
#endif

#ifdef STM32G0
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM17 | LL_APB2_GRP1_PERIPH_ADC);
#else
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_TIM17 | LL_APB1_GRP2_PERIPH_ADC1);
#endif

    ctx_t *ctx = &ctx_;

    led_init();
    led_set(1);
    tim_init();
    uart_init(ctx);

    led_blink(256 * 1024); // initial (on reset) blink

    uint32_t r0 = bl_adc_random_seed();
    ctx->randomseed = r0;

#ifdef OTP_DEVICE_ID_ADDR
    if (*(uint32_t *)OTP_DEVICE_ID_ADDR + 1 == 0) {
        uint32_t r1 = bl_adc_random_seed();
        r0 &= ~0x02000000; // clear "universal" bit
        BL_DEVICE_ID = ((uint64_t)r0 << 32) | r1;
        flash_program((void *)OTP_DEVICE_ID_ADDR, &BL_DEVICE_ID, 8);
    } else {
        BL_DEVICE_ID = *(uint64_t *)OTP_DEVICE_ID_ADDR;
    }
#else
    if ((bl_dev_info.device_id0 + 1) == 0) {
        if (app_dev_info.magic == DEV_INFO_MAGIC && app_dev_info.device_id0 &&
            (app_dev_info.device_id0 + 1)) {
            BL_DEVICE_ID = app_dev_info.device_id;
        } else {
            uint32_t r1 = bl_adc_random_seed();
            r0 &= ~0x02000000; // clear "universal" bit
            BL_DEVICE_ID = ((uint64_t)r0 << 32) | r1;
        }
        flash_program(&bl_dev_info.device_id, &BL_DEVICE_ID, 8);
    } else {
        BL_DEVICE_ID = bl_dev_info.device_id;
    }
#endif

    BL_DEVICE_ID ^= 1; // use different dev-id for application and bootloader

    DMESG("ID: %x %x", (uint32_t)BL_DEVICE_ID, (uint32_t)(BL_DEVICE_ID >> 32));

    ctx->service_class_bl = announce_data[2];
    ctx->next_announce = 1024 * 1024;

    bool app_valid = bl_fixup_app_handlers(ctx);

#if 1
    if (app_valid)
        ctx->app_start_time = 512 * 1024;
    else
#else
    (void)app_valid;
#endif
    ctx->app_start_time = 0x80000000;

    while (1) {
        uint32_t now = ctx->now = tim_get_micros();

        // pin_pulse(PIN_LOG0, 1);

        jd_process(ctx);

        if (now >= ctx->next_announce && !ctx->tx_full) {
            memcpy(ctx->txBuffer.data, announce_data, sizeof(announce_data));
            jd_prep_send(ctx);
            ctx->next_announce = now + 512 * 1024;
        }

        if (now >= ctx->app_start_time)
            start_app();

        if (ctx->led_off_time) {
            if (ctx->led_off_time < now) {
                led_set(0);
                ctx->led_off_time = 0;
            }
        } else {
            led_set((now & 0xff) < ((now & 0x80000) ? 0x4 : 0xf));
        }
    }
}

static void busy_sleep(int ms) {
    ms *= 1000; // check
    while (ms--)
        asm volatile("nop");
}

static void led_panic_blink(void) {
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
