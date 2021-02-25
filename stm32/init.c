#include "jdstm.h"

void tim_update_prescaler(void);

void HardFault_Handler(void) {
    jd_panic();
}

#ifndef BL
void NMI_Handler(void) {}

void SVC_Handler(void) {}

void PendSV_Handler(void) {}

void SysTick_Handler(void) {}
#endif

static void enable_nrst_pin(void) {
#ifdef FLASH_OPTR_NRST_MODE
#define FLASH_KEY1 0x45670123U
#define FLASH_KEY2 0xCDEF89ABU

    DMESG("check NRST", FLASH->OPTR & FLASH_OPTR_NRST_MODE);

    if (FLASH->OPTR & FLASH_OPTR_NRST_MODE_0)
        return;

    uint32_t nrstmode;

    /* Enable Flash access anyway */
    __HAL_RCC_FLASH_CLK_ENABLE();

    /* Unlock flash */
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
    while ((FLASH->CR & FLASH_CR_LOCK) != 0x00)
        ;

    /* unlock option byte registers */
    FLASH->OPTKEYR = 0x08192A3B;
    FLASH->OPTKEYR = 0x4C5D6E7F;
    while ((FLASH->CR & FLASH_CR_OPTLOCK) == FLASH_CR_OPTLOCK)
        ;

    /* get current user option bytes */
    nrstmode = (FLASH->OPTR & ~FLASH_OPTR_NRST_MODE);
    nrstmode |= FLASH_OPTR_NRST_MODE_0;

    /* Program option bytes */
    FLASH->OPTR = nrstmode;

    /* Write operation */
    FLASH->CR |= FLASH_CR_OPTSTRT;
    while ((FLASH->SR & FLASH_SR_BSY1) != 0)
        ;

    /* Force OB Load */
    FLASH->CR |= FLASH_CR_OBL_LAUNCH;

    while (1)
        ;
#endif
}

bool clk_is_pll(void) {
    return LL_RCC_GetSysClkSource() == LL_RCC_SYS_CLKSOURCE_PLL;
}

void clk_setup_pll(void) {
#if defined(STM32G0)
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_1, 8, LL_RCC_PLLR_DIV_2);
#elif defined(STM32F0)
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);

#if defined(STM32F042x6)
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLL_MUL_12, LL_RCC_PREDIV_DIV_2);
#else
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI_DIV_2, LL_RCC_PLL_MUL_12);
#endif
#else
#error "clock?"
#endif

    LL_FLASH_EnablePrefetch();

    LL_RCC_PLL_Enable();
#ifdef RCC_PLLCFGR_PLLREN
    LL_RCC_PLL_EnableDomain_SYS();
#endif
    while (LL_RCC_PLL_IsReady() != 1)
        ;

    // LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);

    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
        ;
}

void clk_set_pll(int on) {
    if (!on) {
        LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
        while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_HSI)
            ;
        cpu_mhz = HSI_MHZ;
        LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
        LL_FLASH_DisablePrefetch();
        tim_update_prescaler();
        return;
    }

    clk_setup_pll();

    cpu_mhz = PLL_MHZ;
    tim_update_prescaler();

    // LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
}

uint8_t cpu_mhz;

void SystemClock_Config(void) {
    cpu_mhz = HSI_MHZ;

    // LL_InitTick(cpu_mhz * 1000000, 1000U);
    // LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);

    enable_nrst_pin();
}

void SystemInit(void) {
#ifdef STM32G0
    SCB->VTOR = FLASH_BASE;
#endif

    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA | LL_AHB1_GRP1_PERIPH_GPIOB |
                             LL_AHB1_GRP1_PERIPH_GPIOC | LL_AHB1_GRP1_PERIPH_GPIOF);

    SystemClock_Config();
}
