#pragma once

#include "jdstm.h"
#include "stm32_mem.h"

#define UTILS_INIT_CRITICAL_SECTION()

#define UTILS_ENTER_CRITICAL_SECTION()                                                             \
    uint32_t _basepri_prev = __get_BASEPRI();                                                      \
    __set_BASEPRI_MAX(IRQ_PRIORITY_LORA << (8U - __NVIC_PRIO_BITS))

#define UTILS_EXIT_CRITICAL_SECTION() __set_BASEPRI(_basepri_prev)
