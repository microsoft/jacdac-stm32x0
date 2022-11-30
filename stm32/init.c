#include "jdstm.h"

void tim_update_prescaler(void);

void HardFault_Handler(void) {
    JD_PANIC();
}

#ifndef BL
void NMI_Handler(void) {}

void SVC_Handler(void) {}

void PendSV_Handler(void) {}

void SysTick_Handler(void) {}
#endif

#define OPTR_MODE0 0x1
#define OPTR_MODE1 0x2
#define OPTR_MODE_LEGACY 0x3

static void enable_nrst_pin(void) {
#ifdef FLASH_OPTR_NRST_MODE
#define FLASH_KEY1 0x45670123U
#define FLASH_KEY2 0xCDEF89ABU

    int msk = (FLASH->OPTR & FLASH_OPTR_NRST_MODE) >> FLASH_OPTR_NRST_MODE_Pos;

#ifdef RESET_AS_GPIO
    // Reset as normal GPIO
    if (msk == OPTR_MODE1)
        return;
#else
    // Reset as conventional reset line
    if (msk == OPTR_MODE0 || msk == OPTR_MODE_LEGACY)
        return;
#endif

    uint32_t nrstmode;

    /* Enable Flash access anyway */
    __HAL_RCC_FLASH_CLK_ENABLE();

    // check no flash op is ongoing
    while ((FLASH->SR & FLASH_SR_BSY1) != 0)
        ;

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

#ifdef RESET_AS_GPIO
    nrstmode |= FLASH_OPTR_NRST_MODE_1;
#else
    nrstmode |= FLASH_OPTR_NRST_MODE_0;
#endif

    /* Program option bytes */
    FLASH->OPTR = nrstmode;
    while ((FLASH->SR & FLASH_SR_BSY1) != 0)
        ;
    // store options via options start
    FLASH->CR |= FLASH_CR_OPTSTRT;
    // wait for complete
    while ((FLASH->SR & FLASH_SR_BSY1) != 0)
        ;

    /* Force OB Load */
    FLASH->CR |= FLASH_CR_OBL_LAUNCH;

    // OBL triggers a reset
    while (1)
        ;
#endif
}

bool clk_is_pll(void) {
    return LL_RCC_GetSysClkSource() == LL_RCC_SYS_CLKSOURCE_PLL;
}

void clk_setup_pll(void) {
#if defined(STM32G0)
    // run at 48 MHz for compatibility with F0
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_1, 6, LL_RCC_PLLR_DIV_2);
#elif defined(STM32F0)
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);

#if defined(STM32F042x6)
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLL_MUL_12, LL_RCC_PREDIV_DIV_2);
#else
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI_DIV_2, LL_RCC_PLL_MUL_12);
#endif
#elif defined(STM32WL)
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_2);
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI, LL_RCC_PLLM_DIV_1, 6, LL_RCC_PLLR_DIV_2);
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

uint8_t cpu_mhz = HSI_MHZ;

#if JD_LORA
uint32_t SystemCoreClock = HSI_MHZ * 1000000;
#define SET_MHZ(n)                                                                                 \
    cpu_mhz = (n);                                                                                 \
    SystemCoreClock = (n)*1000000
#else
#define SET_MHZ(n) cpu_mhz = n
#endif

void clk_set_pll(int on) {
#if defined(STM32WL) || !defined(DISABLE_PLL)
    if (!on) {
        LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);
        while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
            ;
        SET_MHZ(HSI_MHZ);
        LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
        LL_FLASH_DisablePrefetch();
        tim_update_prescaler();
        return;
    }

    clk_setup_pll();

    SET_MHZ(PLL_MHZ);
    tim_update_prescaler();

    // LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
#endif
}

void SystemInit(void) {
    // on G0 this is called before global variables are initialized
    // also, they will be initialized after this is finished, so any writes to globals will be lost

#ifdef STM32G0
    SCB->VTOR = FLASH_BASE; // needed?
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
    LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA | LL_IOP_GRP1_PERIPH_GPIOB |
                            LL_IOP_GRP1_PERIPH_GPIOC);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
#elif defined(STM32WL)
    SCB->VTOR = FLASH_BASE; // needed?
    // LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG); - doesn't have?
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA | LL_AHB2_GRP1_PERIPH_GPIOB |
                             LL_AHB2_GRP1_PERIPH_GPIOC);
    // LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR); - also missing

    // by default MSI is used not HSI16
    LL_RCC_HSI_Enable();
    while (!LL_RCC_HSI_IsReady())
        ;
    clk_set_pll(0);
    LL_RCC_SetClkAfterWakeFromStop(LL_RCC_STOP_WAKEUPCLOCK_HSI);
#else
    LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA | LL_AHB1_GRP1_PERIPH_GPIOB |
                             LL_AHB1_GRP1_PERIPH_GPIOC | LL_AHB1_GRP1_PERIPH_GPIOF);
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
#endif

#ifdef BOARD_STARTUP_CODE
    BOARD_STARTUP_CODE;
#endif
    enable_nrst_pin();
}
