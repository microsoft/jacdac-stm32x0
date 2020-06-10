#include "jd_protocol.h"
#include "interfaces/jd_routing.h"

void app_process_frame() {

    jd_frame_t* frameToHandle = jd_rx_get_frame();

    if (frameToHandle) {
        if (frameToHandle->flags & JD_FRAME_FLAG_ACK_REQUESTED &&
            frameToHandle->flags & JD_FRAME_FLAG_COMMAND &&
            frameToHandle->device_identifier == jd_device_id())
            jd_send(JD_SERVICE_NUMBER_CRC_ACK, frameToHandle->crc, NULL, 0);

        for (;;) {
            app_handle_packet((jd_packet_t *)frameToHandle);
            if (!jd_shift_frame(frameToHandle))
                break;
        }

        frameToHandle = NULL;
    }
}
