
/**
 ******************************************************************************
 * @file    radio_board_if.c
 * @author  MCD Application Team
 * @brief   This file provides an interface layer between MW and Radio Board
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "radio_board_if.h"

/* External variables ---------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int32_t RBI_Init(void) {

#if defined(USE_BSP_DRIVER)
    /* Important note: BSP code is board dependent
     * STM32WL_Nucleo code can be found
     *       either in STM32CubeWL package under Drivers/BSP/STM32WLxx_Nucleo/
     *       or at
     * https://github.com/STMicroelectronics/STM32CubeWL/tree/main/Drivers/BSP/STM32WLxx_Nucleo/ 1/
     * For User boards, the BSP/STM32WLxx_Nucleo/ directory can be copied and replaced in the
     * project. The copy must then be updated depending: on board RF switch configuration (pin
     * control, number of port etc) on TCXO configuration on DC/DC configuration */
    return BSP_RADIO_Init();
#else
    /* 2/ Or implement RBI_Init here */
    int32_t retcode = 0;

#warning user to provide its board code or to call his board driver functions

    return retcode;
#endif /* USE_BSP_DRIVER  */
}

int32_t RBI_DeInit(void) {

#if defined(USE_BSP_DRIVER)
    /* Important note: BSP code is board dependent
     * STM32WL_Nucleo code can be found
     *       either in STM32CubeWL package under Drivers/BSP/STM32WLxx_Nucleo/
     *       or at
     * https://github.com/STMicroelectronics/STM32CubeWL/tree/main/Drivers/BSP/STM32WLxx_Nucleo/ 1/
     * For User boards, the BSP/STM32WLxx_Nucleo/ directory can be copied and replaced in the
     * project. The copy must then be updated depending: on board RF switch configuration (pin
     * control, number of port etc) on TCXO configuration on DC/DC configuration */
    return BSP_RADIO_DeInit();
#else
    /* 2/ Or implement RBI_DeInit here */
    int32_t retcode = 0;

#warning user to provide its board code or to call his board driver functions

    return retcode;
#endif /* USE_BSP_DRIVER */
}

int32_t RBI_ConfigRFSwitch(RBI_Switch_TypeDef Config) {

#if defined(USE_BSP_DRIVER)

    /* Important note: BSP code is board dependent
     * STM32WL_Nucleo code can be found
     *       either in STM32CubeWL package under Drivers/BSP/STM32WLxx_Nucleo/
     *       or at
     * https://github.com/STMicroelectronics/STM32CubeWL/tree/main/Drivers/BSP/STM32WLxx_Nucleo/ 1/
     * For User boards, the BSP/STM32WLxx_Nucleo/ directory can be copied and replaced in the
     * project. The copy must then be updated depending: on board RF switch configuration (pin
     * control, number of port etc) on TCXO configuration on DC/DC configuration */
    return BSP_RADIO_ConfigRFSwitch((BSP_RADIO_Switch_TypeDef)Config);
#else
    /* 2/ Or implement RBI_ConfigRFSwitch here */
    int32_t retcode = 0;

#warning user to provide its board code or to call his board driver functions

    return retcode;
#endif /* USE_BSP_DRIVER */
}

int32_t RBI_GetTxConfig(void) {

#if defined(USE_BSP_DRIVER)
    /* Important note: BSP code is board dependent
     * STM32WL_Nucleo code can be found
     *       either in STM32CubeWL package under Drivers/BSP/STM32WLxx_Nucleo/
     *       or at
     * https://github.com/STMicroelectronics/STM32CubeWL/tree/main/Drivers/BSP/STM32WLxx_Nucleo/ 1/
     * For User boards, the BSP/STM32WLxx_Nucleo/ directory can be copied and replaced in the
     * project. The copy must then be updated depending: on board RF switch configuration (pin
     * control, number of port etc) on TCXO configuration on DC/DC configuration */
    return BSP_RADIO_GetTxConfig();
#else
    /* 2/ Or implement RBI_GetTxConfig here */
    int32_t retcode = RBI_CONF_RFO;

#warning user to provide its board code or to call his board driver functions

    return retcode;
#endif /* USE_BSP_DRIVER */
}

int32_t RBI_IsTCXO(void) {

#if defined(USE_BSP_DRIVER)
    /* Important note: BSP code is board dependent
     * STM32WL_Nucleo code can be found
     *       either in STM32CubeWL package under Drivers/BSP/STM32WLxx_Nucleo/
     *       or at
     * https://github.com/STMicroelectronics/STM32CubeWL/tree/main/Drivers/BSP/STM32WLxx_Nucleo/ 1/
     * For User boards, the BSP/STM32WLxx_Nucleo/ directory can be copied and replaced in the
     * project. The copy must then be updated depending: on board RF switch configuration (pin
     * control, number of port etc) on TCXO configuration on DC/DC configuration */
    return BSP_RADIO_IsTCXO();
#else
    /* 2/ Or implement RBI_IsTCXO here */
    int32_t retcode = IS_TCXO_SUPPORTED;

#warning user to provide its board code or to call his board driver functions

    return retcode;
#endif /* USE_BSP_DRIVER  */
}

int32_t RBI_IsDCDC(void) {

#if defined(USE_BSP_DRIVER)
    /* Important note: BSP code is board dependent
     * STM32WL_Nucleo code can be found
     *       either in STM32CubeWL package under Drivers/BSP/STM32WLxx_Nucleo/
     *       or at
     * https://github.com/STMicroelectronics/STM32CubeWL/tree/main/Drivers/BSP/STM32WLxx_Nucleo/ 1/
     * For User boards, the BSP/STM32WLxx_Nucleo/ directory can be copied and replaced in the
     * project. The copy must then be updated depending: on board RF switch configuration (pin
     * control, number of port etc) on TCXO configuration on DC/DC configuration */
    return BSP_RADIO_IsDCDC();
#else
    /* 2/ Or implement RBI_IsDCDC here */
    int32_t retcode = IS_DCDC_SUPPORTED;

#warning user to provide its board code or to call his board driver functions

    return retcode;
#endif /* USE_BSP_DRIVER  */
}

/* Private Functions Definition -----------------------------------------------*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
