#include "blhw.h"
#include "jd_protocol.h"

uint64_t hw_device_id(void) {
    return app_dev_info.device_id;
}

uint32_t app_get_device_class(void) {
    return app_dev_info.device_class;
}