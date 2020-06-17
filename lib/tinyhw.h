#pragma once

#include "blhw.h"

#ifndef RTC_SECOND_IN_US
// use a little more than 10ms, so we don't have issues with wrap around
#define RTC_SECOND_IN_US 25000
#endif

// exti.c
#define EXTI_FALLING 0x01
#define EXTI_RISING 0x02
void exti_set_callback(uint8_t pin, cb_t callback, uint32_t flags);

// dspi.c
void dspi_init(void);
void dspi_tx(const void *data, uint32_t numbytes, cb_t doneHandler);

#define LIGHT_TYPE_APA_MASK 0x10
#define LIGHT_TYPE_WS2812B_GRB 0x00
#define LIGHT_TYPE_APA102 0x10
#define LIGHT_TYPE_SK9822 0x11

void px_init(int light_type);
void px_tx(const void *data, uint32_t numbytes, uint8_t intensity, cb_t doneHandler);
#define PX_WORDS(NUM_PIXELS) (((NUM_PIXELS)*3 + 3) / 4)

// i2c.c
void i2c_init(void);
// addr are 7bit
int i2c_write_buf(uint8_t addr, const void *src, unsigned len);
int i2c_write_reg_buf(uint8_t addr, uint8_t reg, const void *src, unsigned len);
int i2c_read_buf(uint8_t addr, uint8_t reg, void *dst, unsigned len);
int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val);
int i2c_read_reg(uint8_t addr, uint8_t reg);

// adc.c
void adc_init_random(void);
uint16_t adc_read_temp(void);
bool adc_can_read_pin(uint8_t pin);
void adc_prep_read_pin(uint8_t pin);
uint16_t adc_convert(void);
void adc_disable(void);
uint16_t adc_read_pin(uint8_t pin); // equivalent to the three steps above

// rtc.c
void rtc_init(void);
void rtc_sleep(bool forceShallow);
void rtc_deepsleep(void);
// these three functions have to do with standby mode (where the RTC is the only thing that runs)
void rtc_set_to_seconds_and_standby(void);
uint32_t rtc_get_seconds(void);
bool rtc_check_standby(void);

// pwm.c
uint8_t pwm_init(uint8_t pin, uint32_t period, uint32_t duty, uint8_t prescaler);
void pwm_set_duty(uint8_t pwm_id, uint32_t duty);
void pwm_enable(uint8_t pwm_id, bool enabled);

// bitbang_spi.c
void bspi_send(const void *src, uint32_t len);
void bspi_recv(void *dst, uint32_t len);

// display.c
void disp_show(uint8_t *img);
void disp_refresh(void);
void disp_sleep(void);
int disp_light_level(void);
void disp_set_dark_level(int v);
int disp_get_dark_level(void);

// target_utils.c
void target_reset(void);
RAM_FUNC
void target_wait_cycles(int n);
int target_in_irq(void);

extern uint8_t cpu_mhz;
extern uint16_t tim_max_sleep;
