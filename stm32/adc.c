#include "jdstm.h"
#include "dmesg.h"

static bool adc_calibrated;

static void set_sampling_time(uint32_t time) {
#ifdef STM32G0
    LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_COMMON_1, time);
#else
    LL_ADC_SetSamplingTimeCommonChannels(ADC1, time);
#endif
}

uint16_t adc_convert(void) {
    if ((LL_ADC_IsEnabled(ADC1) == 1) && (LL_ADC_IsDisableOngoing(ADC1) == 0) &&
        (LL_ADC_REG_IsConversionOngoing(ADC1) == 0))
        LL_ADC_REG_StartConversion(ADC1);
    else
        jd_panic();

    while (LL_ADC_IsActiveFlag_EOC(ADC1) == 0)
        ;
    LL_ADC_ClearFlag_EOC(ADC1);

    return LL_ADC_REG_ReadConversionData12(ADC1);
}

static void set_channel(uint32_t chan) {
    // might be from previous disable
    while (LL_ADC_IsDisableOngoing(ADC1))
        ;

#ifdef STM32G0
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, chan);
    LL_ADC_SetChannelSamplingTime(ADC1, chan, LL_ADC_SAMPLINGTIME_COMMON_1);

    LL_ADC_EnableInternalRegulator(ADC1);
    target_wait_us(LL_ADC_DELAY_INTERNAL_REGUL_STAB_US);
#else
    LL_ADC_REG_SetSequencerChannels(ADC1, chan);
#endif

    if (!adc_calibrated) {
        adc_calibrated = 1;
        LL_ADC_StartCalibration(ADC1);

        while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0)
            ;
        target_wait_us(5); // ADC_DELAY_CALIB_ENABLE_CPU_CYCLES
        DMESG("ADC calib: %x", ADC1->DR);
    }

    // LL_ADC_SetLowPowerMode(ADC1, LL_ADC_LP_AUTOWAIT_AUTOPOWEROFF);

    LL_ADC_Enable(ADC1);

    while (LL_ADC_IsActiveFlag_ADRDY(ADC1) == 0)
        ;
}

static void set_temp_ref(int t) {
    while (LL_ADC_IsDisableOngoing(ADC1))
        ;
    LL_ADC_SetCommonPathInternalCh(
        __LL_ADC_COMMON_INSTANCE(ADC1),
        t ? (LL_ADC_PATH_INTERNAL_VREFINT | LL_ADC_PATH_INTERNAL_TEMPSENSOR)
          : LL_ADC_PATH_INTERNAL_NONE);
    if (t)
        target_wait_us(LL_ADC_DELAY_TEMPSENSOR_STAB_US);
}

// 116 bytes
uint32_t bl_adc_random_seed(void) {
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1),
                                   LL_ADC_PATH_INTERNAL_VREFINT | LL_ADC_PATH_INTERNAL_TEMPSENSOR);
    target_wait_us(LL_ADC_DELAY_TEMPSENSOR_STAB_US);

    ADC1->CFGR1 = LL_ADC_REG_OVR_DATA_OVERWRITTEN;

#ifdef STM32G0
    LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_DISABLE);
    target_wait_us(5);

    LL_ADC_REG_SetSequencerConfigurable(ADC1, LL_ADC_REG_SEQ_CONFIGURABLE);
#endif

    // set_sampling_time(LL_ADC_SAMPLINGTIME_1CYCLE_5); // get maximum noise

    ADC1->CFGR2 = LL_ADC_CLOCK_SYNC_PCLK_DIV2;

#ifdef STM32G0
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_TEMPSENSOR);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_TEMPSENSOR, LL_ADC_SAMPLINGTIME_COMMON_1);

    LL_ADC_EnableInternalRegulator(ADC1);
    target_wait_us(LL_ADC_DELAY_INTERNAL_REGUL_STAB_US);
#else
    LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_TEMPSENSOR);
#endif

    // LL_ADC_SetLowPowerMode(ADC1, LL_ADC_LP_AUTOWAIT_AUTOPOWEROFF);

    LL_ADC_Enable(ADC1);
    while (LL_ADC_IsActiveFlag_ADRDY(ADC1) == 0)
        ;

    uint32_t h = 231;
    for (int i = 0; i < 1000; ++i) {
        LL_ADC_REG_StartConversion(ADC1);
        while (LL_ADC_IsActiveFlag_EOC(ADC1) == 0)
            ;
        LL_ADC_ClearFlag_EOC(ADC1);
        int v = LL_ADC_REG_ReadConversionData12(ADC1);
        h = (h * 0x1000193) ^ (v & 0xff);
    }

    return h;
}

// initializes RNG from TEMP sensor
// alternative would be build an RNG, eg http://robseward.com/misc/RNG2/
void adc_init_random(void) {
    __HAL_RCC_ADC_CLK_ENABLE();

    set_temp_ref(1);
    ADC1->CFGR1 = LL_ADC_REG_OVR_DATA_OVERWRITTEN;

#ifdef STM32G0
    LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_DISABLE);
    target_wait_us(5);

    LL_ADC_REG_SetSequencerConfigurable(ADC1, LL_ADC_REG_SEQ_CONFIGURABLE);
#endif

    set_sampling_time(LL_ADC_SAMPLINGTIME_1CYCLE_5); // get maximum noise

    ADC1->CFGR2 = LL_ADC_CLOCK_SYNC_PCLK_DIV2;

    set_channel(LL_ADC_CHANNEL_TEMPSENSOR);

    uint32_t h = 0x811c9dc5;
    for (int i = 0; i < 1000; ++i) {
        int v = adc_convert();
        h = (h * 0x1000193) ^ (v & 0xff);
    }
    jd_seed_random(h);

    // ADC enabled is like 200uA in STOP
    LL_ADC_Disable(ADC1);
    while (LL_ADC_IsDisableOngoing(ADC1))
        ;

    // this brings STOP draw to 4.4uA from 46uA
    set_temp_ref(0);
}

static const uint32_t channels_PA[] = {
    LL_ADC_CHANNEL_0, LL_ADC_CHANNEL_1, LL_ADC_CHANNEL_2, LL_ADC_CHANNEL_3,
    LL_ADC_CHANNEL_4, LL_ADC_CHANNEL_5, LL_ADC_CHANNEL_6, LL_ADC_CHANNEL_7,
};

static const uint32_t channels_PB[] = {
    LL_ADC_CHANNEL_8,
    LL_ADC_CHANNEL_9,
};

#define NO_CHANNEL 0xffffffff

#ifdef STM32F0
#define TS_CAL1 *(uint16_t *)0x1FFFF7B8
#if defined(STM32F031x6) || defined(STM32F042x6)
// not present on F030
#define TS_CAL2 *(uint16_t *)0x1FFFF7C2
#endif
#endif

uint16_t adc_read_temp(void) {
    set_sampling_time(LL_ADC_SAMPLINGTIME_71CYCLES_5); // min. sampling time for temp is 4us
    set_temp_ref(1);
    set_channel(LL_ADC_CHANNEL_TEMPSENSOR);

    uint16_t r = adc_convert();

    LL_ADC_Disable(ADC1);
    set_temp_ref(0);

#if defined(STM32F030x6) || defined(STM32F030x8) 
    return ((TS_CAL1 - r) * 1000) / 5336 + 30;
#elif defined(STM32F031x6) || defined(STM32F042x6)
    return ((110 - 30) * (r - TS_CAL1)) / (TS_CAL2 - TS_CAL1) + 30;
#else
#error "check datasheet!"
#endif
}

static uint32_t pin_channel(uint8_t pin) {
    if (pin >> 4 == 0) {
        if ((pin & 0xf) >= sizeof(channels_PA) / sizeof(channels_PA[0]))
            return NO_CHANNEL;
        return channels_PA[pin & 0xf];
    } else if (pin >> 4 == 1) {
        if ((pin & 0xf) >= sizeof(channels_PB) / sizeof(channels_PB[0]))
            return NO_CHANNEL;
        return channels_PB[pin & 0xf];
    } else {
        return NO_CHANNEL;
    }
}

bool adc_can_read_pin(uint8_t pin) {
    return pin_channel(pin) != NO_CHANNEL;
}

void adc_prep_read_pin(uint8_t pin) {
    uint32_t chan = pin_channel(pin);

    if (chan == NO_CHANNEL)
        jd_panic();

    pin_setup_analog_input(pin);

    set_sampling_time(LL_ADC_SAMPLINGTIME_41CYCLES_5);

    set_channel(chan);
}

void adc_disable() {
    LL_ADC_Disable(ADC1);
}

uint16_t adc_read_pin(uint8_t pin) {
    adc_prep_read_pin(pin);
    uint16_t r = adc_convert();
    adc_disable();
    return r;
}
