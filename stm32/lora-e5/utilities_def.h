
/**
 ******************************************************************************
 * @file    utilities_def.h
 * @author  MCD Application Team
 * @brief   Definitions for modules requiring utilities
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
#ifndef __UTILITIES_DEF_H__
#define __UTILITIES_DEF_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/
/******************************************************************************
 * LOW POWER MANAGER
 ******************************************************************************/
/**
 * Supported requester to the MCU Low Power Manager - can be increased up  to 32
 * It lists a bit mapping of all user of the Low Power Manager
 */
typedef enum {

    CFG_LPM_APPLI_Id,
    CFG_LPM_UART_TX_Id,

} CFG_LPM_Id_t;

/*---------------------------------------------------------------------------*/
/*                             sequencer definitions                         */
/*---------------------------------------------------------------------------*/

/**
 * This is the list of priority required by the application
 * Each Id shall be in the range 0..31
 */
typedef enum {
    CFG_SEQ_Prio_0,

    CFG_SEQ_Prio_NBR,
} CFG_SEQ_Prio_Id_t;

/**
 * This is the list of task id required by the application
 * Each Id shall be in the range 0..31
 */
typedef enum {
    CFG_SEQ_Task_LmHandlerProcess,
    CFG_SEQ_Task_LoRaSendOnTxTimerOrButtonEvent,

    CFG_SEQ_Task_NBR
} CFG_SEQ_Task_Id_t;

/* Exported constants --------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __UTILITIES_DEF_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
