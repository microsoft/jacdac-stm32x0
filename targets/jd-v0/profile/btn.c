#include "jd_services.h"
#include "interfaces/jd_routing.h"
#include "jdprofile.h"

DEVICE_CLASS(0x3d216fd4, "JDF030 btn v0");

void app_init_services(void) {
    btn_init(PA_4, -1);
}
