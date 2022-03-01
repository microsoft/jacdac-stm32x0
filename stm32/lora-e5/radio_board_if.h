
/**
 ******************************************************************************
 * @file    radio_board_if.h
 * @author  MCD Application Team
 * @brief   Header for Radio interface configuration
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef RADIO_BOARD_IF_H
#define RADIO_BOARD_IF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "platform.h"

/* Exported defines ----------------------------------------------------------*/

#define RBI_CONF_RFO_LP_HP 0
#define RBI_CONF_RFO_LP 1
#define RBI_CONF_RFO_HP 2


/* Exported types ------------------------------------------------------------*/

typedef enum {
    RBI_SWITCH_OFF = 0,
    RBI_SWITCH_RX = 1,
    RBI_SWITCH_RFO_LP = 2,
    RBI_SWITCH_RFO_HP = 3,
} RBI_Switch_TypeDef;

/* Exported constants --------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
/**
 * @brief  Init Radio Switch
 * @return BSP status
 */
int32_t RBI_Init(void);

/**
 * @brief  DeInit Radio Switch
 * @return BSP status
 */
int32_t RBI_DeInit(void);

/**
 * @brief  Configure Radio Switch.
 * @param  Config: Specifies the Radio RF switch path to be set.
 *         This parameter can be one of following parameters:
 *           @arg RADIO_SWITCH_OFF
 *           @arg RADIO_SWITCH_RX
 *           @arg RADIO_SWITCH_RFO_LP
 *           @arg RADIO_SWITCH_RFO_HP
 * @return BSP status
 */
int32_t RBI_ConfigRFSwitch(RBI_Switch_TypeDef Config);

/**
 * @brief  Return Board Configuration
 * @retval RBI_CONF_RFO_LP_HP
 * @retval RBI_CONF_RFO_LP
 * @retval RBI_CONF_RFO_HP
 */
int32_t RBI_GetTxConfig(void);

/**
 * @brief  Get If TCXO is to be present on board
 * @note   never remove called by MW,
 * @retval return 1 if present, 0 if not present
 */
int32_t RBI_IsTCXO(void);

/**
 * @brief  Get If DCDC is to be present on board
 * @note   never remove called by MW,
 * @retval return 1 if present, 0 if not present
 */
int32_t RBI_IsDCDC(void);

#ifdef __cplusplus
}
#endif

#endif /* RADIO_BOARD_IF_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
