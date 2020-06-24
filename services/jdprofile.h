#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "board.h"
#include "dmesg.h"
#include "pinnames.h"
#include "jd_services.h"
#include "interfaces/jd_app.h"
#include "blhw.h"

#ifdef BL
#define DEVICE_CLASS(dev_class, dev_class_name)                                                    \
    struct bl_info_block __attribute__((section(".devinfo"), used)) bl_info = {                    \
        .devinfo =                                                                                 \
            {                                                                                      \
                .magic = DEV_INFO_MAGIC,                                                           \
                .device_id = 0xffffffffffffffffULL,                                                \
                .device_class = dev_class,                                                         \
            },                                                                                     \
        .random_seed0 = 0xffffffff,                                                                \
        .random_seed1 = 0xffffffff,                                                                \
        .reserved0 = 0xffffffff,                                                                   \
        .reserved1 = 0xffffffff,                                                                   \
    };
#else
#define DEVICE_CLASS(dev_class, dev_class_name) const char app_dev_class_name[] = dev_class_name;
#endif