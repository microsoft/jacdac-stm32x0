#ifndef __JDSIMPLE_H
#define __JDSIMPLE_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#if defined(STM32F0)
#include "stm32f0.h"
#elif defined(STM32G0)
#include "stm32g0.h"
#else
#error "invalid CPU"
#endif

#include "board.h"
#include "dmesg.h"
#include "pinnames.h"
#include "jdlow.h"
#include "services.h"
#include "host.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RTC_ALRM_US 10000

#define RAM_FUNC __attribute__((noinline, long_call, section(".data")))

// main.c
void led_toggle(void);
void led_set(int state);
void led_blink(int us);
void fail_and_reset(void);

// utils.c
int itoa(int n, char *s);
int string_reverse(char *s);
uint64_t device_id(void);
RAM_FUNC
void target_wait_cycles(int n);
uint32_t random_int(int max);
void dump_pkt(jd_packet_t *pkt, const char *msg);

// exti.c
#define EXTI_FALLING 0x01
#define EXTI_RISING 0x02
void exti_set_callback(uint8_t pin, cb_t callback, uint32_t flags);
void exti_trigger(cb_t cb);
#define exti_disable(pin) LL_EXTI_DisableIT_0_31(pin)
#define exti_enable(pin) LL_EXTI_EnableIT_0_31(pin)
#define exti_enable(pin) LL_EXTI_EnableIT_0_31(pin)
#define exti_clear(pin) LL_EXTI_ClearFallingFlag_0_31(pin)

// dspi.c
void dspi_init(void);
void dspi_tx(const void *data, uint32_t numbytes, cb_t doneHandler);

void px_init(void);
void px_tx(const void *data, uint32_t numbytes, cb_t doneHandler);
void px_set(const void *data, uint32_t index, uint8_t intensity, uint32_t color);
#define PX_WORDS(NUM_PIXELS) (((NUM_PIXELS)*9 + 8) / 4)

// pins.c
void pin_set(int pin, int v);
void pin_setup_output(int pin);
void pin_toggle(int pin);
int pin_get(int pin);
// pull: -1, 0, 1
void pin_setup_input(int pin, int pull);
void pin_setup_output_af(int pin, int af);
void pin_setup_analog_input(int pin);

// adc.c
void adc_init_random(void);

// rtc.c
void rtc_init(void);
void rtc_sleep(void);
void rtc_set_cb(cb_t cb);
void rtc_set_led_duty(int val); // 0-1000

void tim_forward(int us);

// pwm.c
uint8_t pwm_init(uint8_t pin, uint32_t period, uint32_t duty, uint8_t prescaler);
void pwm_set_duty(uint8_t pwm_id, uint32_t duty);

// jdapp.c
void app_process(void);
void app_init_services(void);

// txq.c
void txq_flush(void);
int txq_is_idle(void);
void *txq_push(unsigned service_num, unsigned service_cmd, const void *data, unsigned service_size);

// alloc.c
void alloc_init(void);
void *alloc(uint32_t size);
void alloc_stack_check();

extern uint32_t now;

// check if given timestamp is already in the past, regardless of overflows on 'now'
// the moment has to be no more than ~500 seconds in the past
static inline bool in_past(uint32_t moment) {
    return ((now - moment) >> 29) == 0;
}
static inline bool in_future(uint32_t moment) {
    return ((moment - now) >> 29) == 0;
}

#ifdef __cplusplus
}
#endif

#endif
