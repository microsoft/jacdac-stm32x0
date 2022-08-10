#pragma once

#include "blhw.h"
#include "services/interfaces/jd_adc.h"
#include "jacdac/dist/c/ledstrip.h"

#ifndef RTC_SECOND_IN_US
// use a little more than 10ms, so we don't have issues with wrap around
// also, use value that converts evenly to 32768Hz crystal
#define RTC_SECOND_IN_US 31250
#endif

// exti.c
#define EXTI_FALLING 0x01
#define EXTI_RISING 0x02
void exti_set_callback(uint8_t pin, cb_t callback, uint32_t flags);

// dspi.c (display SPI (for screen and neopixels; with DMA))
void dspi_init(bool slow, int cpol, int cpha);
void dspi_tx(const void *data, uint32_t numbytes, cb_t doneHandler);
void dspi_xfer(const void *txdata, void *rxdata, uint32_t numbytes, cb_t doneHandler);

// spis.c (slave SPI)
bool spis_seems_connected(void);
void spis_init(void);
void spis_xfer(const void *txdata, void *rxdata, uint32_t numbytes, cb_t doneHandler);
void spis_log(void);
void spis_abort(void);
extern cb_t spis_halftransfer_cb;
extern cb_t spis_error_cb;

// rtc.c
void rtc_init(void);
void rtc_sleep(bool forceShallow);
void rtc_deepsleep(void);
// these three functions have to do with standby mode (where the RTC is the only thing that runs)
void rtc_set_to_seconds_and_standby(void);
uint32_t rtc_get_seconds(void);
bool rtc_check_standby(void);

// display.c
#ifndef DISP_LIGHT_SENSE
#define DISP_LIGHT_SENSE 1
#endif
void disp_show(uint8_t *img);
void disp_refresh(void);
void disp_sleep(void);
#if DISP_LIGHT_SENSE
int disp_light_level(void);
void disp_set_dark_level(int v);
int disp_get_dark_level(void);
#else
void disp_set_brigthness(uint16_t v);
#endif

// duart.c
typedef void (*data_cb_t)(const uint8_t *buf, unsigned size);
void duart_init(data_cb_t rx_cb);
void duart_start_tx(const void *data, uint32_t numbytes, cb_t done_handler);
void duart_process(void);

void usbserial_init(void);

// target_utils.c
void target_reset(void);
RAM_FUNC
void target_wait_cycles(int n);
