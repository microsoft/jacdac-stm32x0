#include "jdsimple.h"

#define CHECK_PERIOD 1000
#define OVERLOAD_MS 1000

#define MA_SCALE 2995
#define MV_SCALE 207

#define PIN_POST_SENSE PA_3
#define PIN_PRE_SENSE PA_4
#define PIN_GND_SENSE PA_5
#define PIN_OVERLOAD PA_6

#define PWR_REG_BAT_VOLTAGE 0x180
#define PWR_REG_OVERLOAD 0x181

#define READING_WINDOW 3

struct srv_state {
    SENSOR_COMMON;
    uint8_t intensity;
    uint8_t overload;
    uint16_t max_power;
    uint16_t curr_power;
    uint16_t battery_voltage;
    uint32_t nextSample;
    uint32_t overloadExpire;
    uint16_t nsum;
    uint32_t sum_gnd;
    uint32_t sum_pre;
    uint16_t readings[READING_WINDOW - 1];
    uint8_t pwr_on;
};

REG_DEFINITION(                   //
    power_regs,                   //
    REG_SENSOR_BASE,              //
    REG_U8(JD_REG_INTENSITY),     //
    REG_U8(PWR_REG_OVERLOAD),     //
    REG_U16(JD_REG_MAX_POWER),    //
    REG_U16(JD_REG_READING),      //
    REG_U16(PWR_REG_BAT_VOLTAGE), //
)

static void sort_ints(uint16_t arr[], int n) {
    for (int i = 1; i < n; i++)
        for (int j = i; j > 0 && arr[j - 1] > arr[j]; j--) {
            int tmp = arr[j - 1];
            arr[j - 1] = arr[j];
            arr[j] = tmp;
        }
}

static void overload(srv_t *state) {
    pwr_pin_enable(0);
    state->pwr_on = 0;
    state->overloadExpire = now + OVERLOAD_MS * 1000;
    state->overload = 1;
}

static int overload_detect(srv_t *state, uint16_t readings[READING_WINDOW]) {
    sort_ints(readings, READING_WINDOW);
    int med_gnd = readings[READING_WINDOW / 2];
    int ma = (med_gnd * MA_SCALE) >> 7;
    if (ma > state->max_power) {
        overload(state);
        return 1;
    }
    return 0;
}

static void turn_on_power(srv_t *state) {
    DMESG("power on");
    uint16_t readings[READING_WINDOW] = {0};
    unsigned rp = 0;
    uint32_t t0 = tim_get_micros();
    pwr_pin_enable(1);
    for (int i = 0; i < 10000; ++i) {
        int gnd = adc_read_pin(PIN_GND_SENSE);
        readings[rp++] = gnd;
        if (rp == READING_WINDOW)
            rp = 0;
        if (overload_detect(state, readings)) {
            DMESG("overload after %d readings %d us", i+1, (uint32_t)tim_get_micros() - t0);
            return;
        }
    }
    state->pwr_on = 1;
}

void power_process(srv_t *state) {
    sensor_process_simple(state, &state->curr_power, sizeof(state->curr_power));

    if (!should_sample(&state->nextSample, CHECK_PERIOD * 9 / 10))
        return;

    if (state->overload && in_past(state->overloadExpire))
        state->overload = 0;
    pin_set(PIN_OVERLOAD, state->overload);

    int should_be_on = state->intensity && !state->overload;
    if (should_be_on != state->pwr_on) {
        if (should_be_on) {
            turn_on_power(state);
        } else {
            DMESG("power off");
            state->pwr_on = 0;
            pwr_pin_enable(0);
        }
    }

    int gnd = adc_read_pin(PIN_GND_SENSE);
    int pre = adc_read_pin(PIN_PRE_SENSE);

    uint16_t sortedReadings[READING_WINDOW];
    memcpy(sortedReadings + 1, state->readings, sizeof(state->readings));
    sortedReadings[0] = gnd;
    memcpy(state->readings, sortedReadings, sizeof(state->readings));

    if (overload_detect(state, sortedReadings)) {
        DMESG("overload detected");
    }

    state->nsum++;
    state->sum_gnd += gnd;
    state->sum_pre += pre;

    if (state->nsum >= 1000) {
        state->curr_power = (state->sum_gnd * MA_SCALE) / (128 * state->nsum);
        state->battery_voltage = (state->sum_pre * 162) / (128 * state->nsum);

        DMESG("%dmV %dmA", state->battery_voltage, state->curr_power);

        state->nsum = 0;
        state->sum_gnd = 0;
        state->sum_pre = 0;
    }
}

void power_handle_packet(srv_t *state, jd_packet_t *pkt) {
    if (sensor_handle_packet_simple(state, pkt, &state->curr_power, sizeof(state->curr_power)))
        return;

    int r = srv_handle_reg(state, pkt, power_regs);
    (void)r;
}

SRV_DEF(power, JD_SERVICE_CLASS_POWER);
void power_init(void) {
    SRV_ALLOC(power);
    state->intensity = 1;
    state->max_power = 500;
    tim_max_sleep = CHECK_PERIOD;
    pin_setup_output(PIN_OVERLOAD);
}
