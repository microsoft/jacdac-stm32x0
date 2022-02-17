
/**
 ******************************************************************************
 * @file    systime.h
 * @author  MCD Application Team
 * @brief   Map middleware systime
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
#ifndef __SYSTIME_H__
#define __SYSTIME_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


/*!
* @brief Number of seconds elapsed between Unix epoch and GPS epoch
*/
#define UNIX_GPS_EPOCH_OFFSET                       315964800


typedef struct SysTime_s
{
uint32_t Seconds;
int16_t SubSeconds;
}SysTime_t;

/*!
* @brief Gets current system time
*
* @retval sysTime    Current seconds/sub-seconds since UNIX epoch origin
*/
SysTime_t SysTimeGet( void );

/*!
* @brief Gets current MCU system time
*
* @retval sysTime    Current seconds/sub-seconds since Mcu started
*/
SysTime_t SysTimeGetMcuTime( void );

/*!
* Converts the given SysTime to the equivalent RTC value in milliseconds
*
* @param [IN] sysTime System time to be converted
* 
* @retval timeMs The RTC converted time value in ms
*/
uint32_t SysTimeToMs( SysTime_t sysTime );

/*!
* Converts the given RTC value in milliseconds to the equivalent SysTime
*
* \param [IN] timeMs The RTC time value in ms to be converted
* 
* \retval sysTime Converted system time
*/
SysTime_t SysTimeFromMs( uint32_t timeMs );

/*!
* @brief Adds 2 SysTime_t values
*
* @param a Value
* @param b Value to added
*
* @retval result Addition result (SysTime_t value)
*/
SysTime_t SysTimeAdd( SysTime_t a, SysTime_t b );

/*!
* @brief Subtracts 2 SysTime_t values
*
* @param a Value
* @param b Value to be subtracted
*
* @retval result Subtraction result (SysTime_t value)
*/
SysTime_t SysTimeSub( SysTime_t a, SysTime_t b );

/*!
* @brief Sets new system time
*
* @param  sysTime    New seconds/sub-seconds since UNIX epoch origin
*/
void SysTimeSet( SysTime_t sysTime );


#ifdef __cplusplus
}
#endif

#endif /*__SYSTIME_H__*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
