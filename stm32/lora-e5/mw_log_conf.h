#pragma once

#include "jd_user_config.h"

#define MW_LOG(ts, vl, fmt, ...) DMESG("MW: " fmt, ##__VA_ARGS__)
