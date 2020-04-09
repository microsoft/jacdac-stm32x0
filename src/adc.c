#include "jdsimple.h"

uint16_t adc_convert() {
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

// alternative would be build an RNG, eg http://robseward.com/misc/RNG2/

// initializes RNG from TEMP sensor
void adc_init_random(void) {
    __HAL_RCC_ADC_CLK_ENABLE();

    LL_ADC_SetCommonPathInternalCh(
        __LL_ADC_COMMON_INSTANCE(ADC1),
        (LL_ADC_PATH_INTERNAL_VREFINT | LL_ADC_PATH_INTERNAL_TEMPSENSOR));
    target_wait_us(LL_ADC_DELAY_TEMPSENSOR_STAB_US);

    ADC1->CFGR1 = LL_ADC_REG_OVR_DATA_OVERWRITTEN;

#ifdef STM32G0
    LL_ADC_REG_SetSequencerLength(ADC1, LL_ADC_REG_SEQ_SCAN_DISABLE);
    target_wait_us(5);

    LL_ADC_REG_SetSequencerConfigurable(ADC1, LL_ADC_REG_SEQ_CONFIGURABLE);
    LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_COMMON_1,
                                         LL_ADC_SAMPLINGTIME_1CYCLE_5);
#else
    LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_1CYCLE_5);
#endif

    ADC1->CFGR2 = LL_ADC_CLOCK_SYNC_PCLK_DIV2;

#ifdef STM32G0
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_TEMPSENSOR);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_TEMPSENSOR, LL_ADC_SAMPLINGTIME_COMMON_1);

    LL_ADC_EnableInternalRegulator(ADC1);
    target_wait_us(LL_ADC_DELAY_INTERNAL_REGUL_STAB_US);
#else
    LL_ADC_REG_SetSequencerChannels(ADC1, LL_ADC_CHANNEL_TEMPSENSOR);
#endif

    LL_ADC_StartCalibration(ADC1);

    while (LL_ADC_IsCalibrationOnGoing(ADC1) != 0)
        ;
    target_wait_us(5); // ADC_DELAY_CALIB_ENABLE_CPU_CYCLES

    // LL_ADC_SetLowPowerMode(ADC1, LL_ADC_LP_AUTOPOWEROFF);

    LL_ADC_Enable(ADC1);

    while (LL_ADC_IsActiveFlag_ADRDY(ADC1) == 0)
        ;

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
    LL_ADC_REG_SetSequencerChannels(ADC1, 0);
    LL_ADC_SetCommonPathInternalCh(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_PATH_INTERNAL_NONE);
    target_wait_us(LL_ADC_DELAY_TEMPSENSOR_STAB_US);

    __HAL_RCC_ADC_CLK_DISABLE();
}
