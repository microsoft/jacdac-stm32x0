#include "bl.h"

static void start_app(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
    BL_MAGIC_FLAG = BL_MAGIC_FLAG_APP;
#pragma GCC diagnostic pop
    target_reset();
}

ctx_t ctx_;

// The LED_BL will be run at 10/BL_LED_PERIOD and 30/BL_LED_PERIOD
#ifndef BL_LED_PERIOD
#define BL_LED_PERIOD 300
#endif

#ifdef LED_RGB_COMMON_CATHODE
#define SET_LED(v) blled_set_duty(v)
#else
#define SET_LED(v) blled_set_duty(BL_LED_PERIOD - (v))
#endif

void led_init(void) {
#ifdef PIN_BL_LED
    SET_LED(0);
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

#ifndef PIN_BL_LED
void led_set(int state) {
    pin_set(PIN_LED, state);
}
#endif

void led_blink(int us) {
    ctx_.led_on_time = tim_get_micros() + us;
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
    // clk_setup_pll();

#if USART_IDX == 1
#ifdef STM32G0
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
#else
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_USART1);
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
    tim_init();
    uart_init(ctx);

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

    // wait a tiny bit, to desynchronize various bootloaders (so that eg PWM don't fire in sync)
    target_wait_us(random(ctx) & 0xff);

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
    // we delay the first LED light up randomly, so it's not very likely to get synchronized with
    // other bootloaders
    uint32_t led_cnt_down = (ctx->randomseed & 0xff) + 10;

    while (1) {
        uint32_t now = ctx->now = tim_get_micros();

        LOG1_PULSE();

        if (jd_process(ctx))
            continue;

        if (!ctx->tx_full) {
            if (now >= ctx->next_announce) {
                memcpy(ctx->txBuffer.data, announce_data, sizeof(announce_data));
                jd_prep_send(ctx);
                ctx->next_announce = now + 512 * 1024;
            } else if (ctx->id_queued) {
                ((uint32_t *)ctx->txBuffer.data)[0] =
                    4 | (JD_SERVICE_NUMBER_CONTROL << 8) | (ctx->id_queued << 16);
                ((uint32_t *)ctx->txBuffer.data)[1] = bl_dev_info.device_class;
                ctx->id_queued = 0;
                jd_prep_send(ctx);
            }
        }

        if (now >= ctx->app_start_time)
            start_app();

        led_cnt_down--;
#ifdef PIN_BL_LED
        if (led_cnt_down == 0) {
            led_cnt_down = 10;
            if (ctx->led_on_time < now) {
                if (now & 0x80000)
                    SET_LED(30);
                else
                    SET_LED(10);
            } else {
                SET_LED(0);
            }
        }
#else
        if (led_cnt_down == 1) {
            if (ctx->led_on_time < now) {
                led_set(1);
            }
        } else if (led_cnt_down == 0) {
            led_set(0);
            if (now & 0x80000)
                led_cnt_down = 5;
            else
                led_cnt_down = 10;
        }
#endif
    }
}

static void busy_sleep(int ms) {
    ms *= 1000; // check
    while (ms--)
        asm volatile("nop");
}

static void led_panic_blink(void) {
#ifdef PIN_BL_LED
    SET_LED(40);
    busy_sleep(70);
    SET_LED(0);
    busy_sleep(70);
#else
    led_set(1);
    busy_sleep(70);
    led_set(0);
    busy_sleep(70);
#endif
}

void jd_panic(void) {
    DMESG("PANIC!");
    while (1) {
        led_panic_blink();
    }
}
