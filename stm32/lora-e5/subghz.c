/**
 ******************************************************************************
 * @file    subghz.c
 * @brief   This file provides code for the configuration
 *          of the SUBGHZ instances.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */

#include "jdstm.h"
#include "subghz.h"

SUBGHZ_HandleTypeDef hsubghz;

void MX_SUBGHZ_Init(void) {
    hsubghz.Init.BaudratePrescaler = SUBGHZSPI_BAUDRATEPRESCALER_4;
    if (HAL_SUBGHZ_Init(&hsubghz) != HAL_OK) {
        jd_panic();
    }
}

void HAL_SUBGHZ_MspInit(SUBGHZ_HandleTypeDef *subghzHandle) {
    __HAL_RCC_SUBGHZSPI_CLK_ENABLE();
    NVIC_SetPriority(SUBGHZ_Radio_IRQn, IRQ_PRIORITY_LORA);
    NVIC_EnableIRQ(SUBGHZ_Radio_IRQn);
}

void HAL_SUBGHZ_MspDeInit(SUBGHZ_HandleTypeDef *subghzHandle) {
    __HAL_RCC_SUBGHZSPI_CLK_DISABLE();
    NVIC_DisableIRQ(SUBGHZ_Radio_IRQn);
}

void SUBGHZ_Radio_IRQHandler(void) {
    HAL_SUBGHZ_IRQHandler(&hsubghz);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
