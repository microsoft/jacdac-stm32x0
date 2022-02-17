/*!
 * \file      systime.c
 *
 * \brief     System time functions implementation.
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2018 Semtech - STMicroelectronics
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 *
 * \author    MCD Application Team ( STMicroelectronics International )
 */
/**
******************************************************************************
* @file  stm32_systime.c
* @author  MCD Application Team
* @brief   System time functions implementation
******************************************************************************
* @attention
*
* <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
* All rights reserved.</center></h2>
*
* This software component is licensed by ST under BSD 3-Clause license,
* the "License"; You may not use this file except in compliance with the
* License. You may obtain a copy of the License at:
*            opensource.org/licenses/BSD-3-Clause
*
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "jdstm.h"
#include "systime.h"

/* Functions Definition ------------------------------------------------------*/
/**
 * @addtogroup SYSTIME_exported_function
 *  @{
 */

SysTime_t SysTimeAdd(SysTime_t a, SysTime_t b) {
    SysTime_t c = {.Seconds = 0, .SubSeconds = 0};

    c.Seconds = a.Seconds + b.Seconds;
    c.SubSeconds = a.SubSeconds + b.SubSeconds;
    if (c.SubSeconds >= 1000) {
        c.Seconds++;
        c.SubSeconds -= 1000;
    }
    return c;
}

SysTime_t SysTimeSub(SysTime_t a, SysTime_t b) {
    SysTime_t c = {.Seconds = 0, .SubSeconds = 0};

    c.Seconds = a.Seconds - b.Seconds;
    c.SubSeconds = a.SubSeconds - b.SubSeconds;
    if (c.SubSeconds < 0) {
        c.Seconds--;
        c.SubSeconds += 1000;
    }
    return c;
}

void SysTimeSet(SysTime_t sysTime) {
    SysTime_t DeltaTime;

    SysTime_t calendarTime = {.Seconds = 0, .SubSeconds = 0};

    calendarTime.Seconds = UTIL_SYSTIMDriver.GetCalendarTime((uint16_t *)&calendarTime.SubSeconds);

    // sysTime is UNIX epoch
    DeltaTime = SysTimeSub(sysTime, calendarTime);

    UTIL_SYSTIMDriver.BKUPWrite_Seconds(DeltaTime.Seconds);
    UTIL_SYSTIMDriver.BKUPWrite_SubSeconds((uint32_t)DeltaTime.SubSeconds);
}

SysTime_t SysTimeGet(void) {
    SysTime_t calendarTime = {.Seconds = 0, .SubSeconds = 0};
    SysTime_t sysTime = {.Seconds = 0, .SubSeconds = 0};
    SysTime_t DeltaTime;

    calendarTime.Seconds = UTIL_SYSTIMDriver.GetCalendarTime((uint16_t *)&calendarTime.SubSeconds);

    DeltaTime.SubSeconds = (int16_t)UTIL_SYSTIMDriver.BKUPRead_SubSeconds();
    DeltaTime.Seconds = UTIL_SYSTIMDriver.BKUPRead_Seconds();

    sysTime = SysTimeAdd(DeltaTime, calendarTime);

    return sysTime;
}

SysTime_t SysTimeGetMcuTime(void) {
    SysTime_t calendarTime = {.Seconds = 0, .SubSeconds = 0};

    calendarTime.Seconds = UTIL_SYSTIMDriver.GetCalendarTime((uint16_t *)&calendarTime.SubSeconds);

    return calendarTime;
}

uint32_t SysTimeToMs(SysTime_t sysTime) {
    SysTime_t DeltaTime;
    DeltaTime.SubSeconds = (int16_t)UTIL_SYSTIMDriver.BKUPRead_SubSeconds();
    DeltaTime.Seconds = UTIL_SYSTIMDriver.BKUPRead_Seconds();

    SysTime_t calendarTime = SysTimeSub(sysTime, DeltaTime);
    return calendarTime.Seconds * 1000 + calendarTime.SubSeconds;
}

SysTime_t SysTimeFromMs(uint32_t timeMs) {
    uint32_t seconds = timeMs / 1000;
    SysTime_t sysTime = {.Seconds = seconds, .SubSeconds = timeMs - seconds * 1000};
    SysTime_t DeltaTime = {0};

    DeltaTime.SubSeconds = (int16_t)UTIL_SYSTIMDriver.BKUPRead_SubSeconds();
    DeltaTime.Seconds = UTIL_SYSTIMDriver.BKUPRead_Seconds();
    return SysTimeAdd(sysTime, DeltaTime);
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/