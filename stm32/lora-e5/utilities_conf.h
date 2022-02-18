#pragma once

#include "cmsis_compiler.h"
#include "stm32_mem.h"
#include <stddef.h>

#define LORA_PRIORITY 3

#define UTILS_INIT_CRITICAL_SECTION()

#define UTILS_ENTER_CRITICAL_SECTION()                                                             \
    uint32_t _basepri_prev = __get_BASEPRI();                                                      \
    __set_BASEPRI_MAX(LORA_PRIORITY << (8U - __NVIC_PRIO_BITS))

#define UTILS_EXIT_CRITICAL_SECTION() __set_BASEPRI(_basepri_prev)
