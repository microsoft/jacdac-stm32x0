#include "bl.h"

struct device_info_block __attribute__((section(".devinfo"), used)) bl_dev_info = {
    .magic = DEV_INFO_MAGIC,
    .device_id = 0xffffffffffffffffULL,
    .device_type = HW_TYPE,
};

static void start_app() {
    BL_MAGIC_FLAG = BL_MAGIC_FLAG_APP;
    target_reset();
}

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

    led_blink(256 * 1024); // initial (on reset) blink

    uint8_t *membase = (uint8_t)0x20000000;

    uint32_t r0 = hash(membase + 0, 4096);

    ctx->randomseed = r0;
    random(ctx); // rotate

    bool app_valid = app_dev_info.magic == DEV_INFO_MAGIC;

    if ((bl_dev_info.device_id + 1) == 0) {
        if (app_valid && app_dev_info.device_id && (app_dev_info.device_id + 1)) {
            BL_DEVICE_ID = app_dev_info.device_id;
        } else {
            uint32_t r1 = hash(membase + 2048, 2048);
            BL_DEVICE_ID = ((uint64_t)r0 << 32) | r1;
        }
        flash_program(&bl_dev_info.device_id, &BL_DEVICE_ID, 8);
    } else {
        BL_DEVICE_ID = bl_dev_info.device_id;
    }

    BL_DEVICE_ID++; // use different dev-id for application and bootloader

    if (!app_valid)
        app_valid = bl_fixup_app_handlers(ctx);

    DMESG("ID: %x %x", (uint32_t)BL_DEVICE_ID, (uint32_t)(BL_DEVICE_ID >> 32));

    ctx->service_class_bl = announce_data[2];
    ctx->next_announce = 128 * 1024; // TODO change it, so we don't announce normally

    if (app_valid)
        ctx->app_start_time = 512 * 1024;
    else
        ctx->app_start_time = 0x80000000;

    while (1) {
        uint32_t now = ctx->now = tim_get_micros();

        jd_process(ctx);

        if (now >= ctx->next_announce && !ctx->tx_full) {
            memcpy(ctx->txBuffer.data, announce_data, sizeof(announce_data));
            ctx->tx_full = 1;
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
