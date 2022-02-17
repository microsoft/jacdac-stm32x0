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

#include "jdstm.h"
#include "systime.h"
#include <stdlib.h> // lldiv()

static SysTime_t _DeltaTime;

static void save_delta(SysTime_t delta) {
    // TODO save in BKP
    _DeltaTime = delta;
}

static SysTime_t read_delta(void) {
    return _DeltaTime;
}

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
    // sysTime is UNIX epoch
    save_delta(SysTimeSub(sysTime, SysTimeGetMcuTime()));
}

SysTime_t SysTimeGet(void) {
    return SysTimeAdd(read_delta(), SysTimeGetMcuTime());
}

uint32_t SysTimeToMs(SysTime_t sysTime) {
    SysTime_t calendarTime = SysTimeSub(sysTime, read_delta());
    return calendarTime.Seconds * 1000 + calendarTime.SubSeconds;
}

SysTime_t SysTimeFromMs(uint32_t timeMs) {
    uint32_t seconds = timeMs / 1000;
    SysTime_t sysTime = {.Seconds = seconds, .SubSeconds = timeMs - seconds * 1000};
    return SysTimeAdd(sysTime, read_delta());
}

SysTime_t SysTimeGetMcuTime(void) {
    uint64_t now = tim_get_micros();

#if 1
    // this will be 500-1000 cycles, so use this function sparingly
    lldiv_t r = lldiv(now, 1000000);
    SysTime_t s = {.Seconds = r.quot, .SubSeconds = (uint32_t)r.rem / 1000};
    return s;
#else
    static SysTime_t prevSysTime;
    static uint64_t prevUS;

    // first call?
    if (prevUS == 0) {
        prevUS = now;
        prevSysTime.Seconds = 1;
        return prevSysTime;
    }

    int64_t delta = now - prevUS;
    if (delta <= 0) {
        // time went back? just return the same time
        return prevSysTime;
    }

    // only allow ~hour between calls
    if (delta >> 32)
        jd_panic();

    uint32_t subs = (uint32_t)delta / 1000;
    if (subs > 1000) {
        prevSysTime.Seconds += subs / 1000;
        prevSysTime.SubSeconds += subs % 1000;
    } else {
        prevSysTime.SubSeconds += subs;
    }
    if (prevSysTime.SubSeconds >= 1000) {
        prevSysTime.SubSeconds -= 1000;
        prevSysTime.Seconds++;
    }

    return prevSysTime;
#endif
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
