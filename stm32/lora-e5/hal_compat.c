#include "jdstm.h"

void HAL_Delay(uint32_t Delay) {
    if (Delay > 500)
        JD_PANIC();
    target_wait_us(Delay * 1000);
}
