#pragma once

#include "blhw.h"
#include "services/interfaces/jd_adc.h"
#include "jacdac/dist/c/ledpixel.h"

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
void dspi_xfer(const void *txdata, void *rxdata, uint32_t numbytes, cb_t doneHandler);

// sspic.c
void sspi_init(void);
void sspi_tx(uint8_t *data, uint32_t numbytes);
void sspi_rx(uint8_t *buf, uint32_t numbytes);

#define LIGHT_TYPE_APA_MASK 0x10
#define LIGHT_TYPE_WS2812B_GRB 0x00
#define LIGHT_TYPE_APA102 0x10
#define LIGHT_TYPE_SK9822 0x11

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

// display.c
void disp_show(uint8_t *img);
void disp_refresh(void);
void disp_sleep(void);
int disp_light_level(void);
void disp_set_dark_level(int v);
int disp_get_dark_level(void);

// Things below are only required by drivers/*.c

// i2c.c
// addr are always 7bit
void i2c_init(void);
int i2c_setup_write(uint8_t addr, unsigned len, bool repeated);
int i2c_write(uint8_t c);
int i2c_finish_write(bool repeated);

// utilities, 8-bit register addresses
int i2c_write_reg_buf(uint8_t addr, uint8_t reg, const void *src, unsigned len);
int i2c_read_reg_buf(uint8_t addr, uint8_t reg, void *dst, unsigned len);
int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val);
int i2c_read_reg(uint8_t addr, uint8_t reg);

// 16-bit reg addresses
int i2c_write_reg16_buf(uint8_t addr, uint16_t reg, const void *src, unsigned len);
int i2c_read_reg16_buf(uint8_t addr, uint16_t reg, void *dst, unsigned len);
// note that this still reads/writes a single byte, from a 16 bit address
int i2c_write_reg16(uint8_t addr, uint16_t reg, uint8_t val);
int i2c_read_reg16(uint8_t addr, uint16_t reg);

// mbed-style
int i2c_write_ex(uint8_t addr, const void *src, unsigned len, bool repeated);
int i2c_read_ex(uint8_t addr, void *dst, unsigned len);

// target_utils.c
void target_reset(void);
RAM_FUNC
void target_wait_cycles(int n);
