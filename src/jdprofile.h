#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "jd_protocol.h"
#include "services/jd_services.h"
#include "dmesg.h"
#include "pinnames.h"
#include "blhw.h"

#ifdef BL
#define FIRMWARE_IDENTIFIER(dev_class, dev_class_name)                                             \
    struct bl_info_block __attribute__((section(".devinfo"), used)) bl_info = {                    \
        .devinfo =                                                                                 \
            {                                                                                      \
                .magic = DEV_INFO_MAGIC,                                                           \
                .device_id = 0xffffffffffffffffULL,                                                \
                .device_class = dev_class,                                                         \
            },                                                                                     \
        .reserved = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff},                              \
    };
#else
#define FIRMWARE_IDENTIFIER(dev_class, dev_class_name)                                             \
    const char app_dev_class_name[] = dev_class_name;
#endif

#ifdef BL
void board_init(void);
#else
#define PROFILE_INIT 1
#include "board.h" // again
#endif
