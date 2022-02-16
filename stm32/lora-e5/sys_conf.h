
/**
 ******************************************************************************
 * @file    sys_conf.h
 * @author  MCD Application Team
 * @brief   Applicative configuration, e.g. : debug, trace, low power, sensors
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
#ifndef __SYS_CONF_H__
#define __SYS_CONF_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/**
 * @brief Temperature and pressure values are retrieved from sensors shield
 *        (instead of sending dummy values). It requires MEMS IKS shield
 */
#define SENSOR_ENABLED 0

/**
 * @brief  Verbose level for all trace logs
 */
#define VERBOSE_LEVEL VLEVEL_H

/**
 * @brief Enable trace logs
 */
#define APP_LOG_ENABLED 1

/**
 * @brief Enable MCU Debugger pins (dbg serial wires, sbg spi, etc)
 */
#define DEBUGGER_ENABLED 1

/**
 * @brief Enable four wires usable as probes (two of them PROBE1 and PROBE2 used by the MW)
 */
#define PROBE_PINS_ENABLED 0

/**
 * @brief Disable Low Power mode
 * @note  0: LowPowerMode enabled. MCU enters stop2 mode, 1: LowPowerMode disabled. MCU enters sleep
 * mode only
 */
#define LOW_POWER_DISABLE 1

/* External variables --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __SYS_CONF_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
