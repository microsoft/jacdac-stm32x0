#pragma once

#include "hwconfig.h"

#define CONCAT_1(a, b) a##b
#define CONCAT_0(a, b) CONCAT_1(a, b)
#ifndef STATIC_ASSERT
#define STATIC_ASSERT(e) enum { CONCAT_0(_static_assert_, __LINE__) = 1 / ((e) ? 1 : 0) };
#endif

#define RAM_FUNC __attribute__((noinline, long_call, section(".data")))

// pins.c
void _pin_set(int pin, int v);
static inline void pin_set(int pin, int v) {
    if ((uint8_t)pin == 0xff)
        return;
    _pin_set(pin, v);
}
void pin_setup_output(int pin);
void pin_set_opendrain(int pin);
void pin_toggle(int pin);
int pin_get(int pin);
// pull: -1, 0, 1
void pin_setup_input(int pin, int pull);
void pin_setup_output_af(int pin, int af);
void pin_setup_analog_input(int pin);
void pin_pulse(int pin, int times);

// init.c
bool clk_is_pll(void);
void clk_set_pll(int on);
void clk_setup_pll(void);

// flash.c
void flash_program(void *dst, const void *src, uint32_t len);
void flash_erase(void *page_addr);

void jd_panic(void);
void target_reset(void);
void target_wait_us(uint32_t n);

#define DEV_INFO_MAGIC 0xf6a0e4b6

struct device_info_block {
    uint32_t magic;
    uint32_t device_class;
    uint64_t device_id;
} __attribute__((packed, aligned(4)));


struct bl_info_block {
    struct device_info_block devinfo;
    uint32_t random_seed0;
    uint32_t random_seed1;
    uint32_t reserved0;
    uint32_t reserved1;
} __attribute__((packed, aligned(4)));

extern struct bl_info_block bl_info;
#define bl_dev_info bl_info.devinfo

STATIC_ASSERT(sizeof(struct device_info_block) == 16);

struct app_top_handlers {
    uint32_t stack_bottom;
    uint32_t boot_reset_handler;
    uint32_t nmi_handler;
    uint32_t hardfault_handler;
    uint32_t memfault_handler;   // not used on M0
    uint32_t busfault_handler;   // not used on M0
    uint32_t usagefault_handler; // not used on M0

    // next 4 words are reserved; we (ab)use them
    struct device_info_block devinfo;

    uint32_t svc_handler;
    uint32_t reserved_for_dbg;

    uint32_t app_reset_handler; // we also abuse this one

    uint32_t pendsv_handler;
    uint32_t systick_handler;
    uint32_t irq_handler[];
};

#define app_handlers ((struct app_top_handlers *)0x8000000)
#define app_dev_info app_handlers->devinfo

static inline uint64_t device_id(void) {
    return app_dev_info.device_id;
}

#ifndef BL
#define bl_info (*((struct bl_info_block*)(0x8000000 + FLASH_SIZE - BL_SIZE)))
#endif
