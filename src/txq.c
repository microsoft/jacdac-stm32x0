#include "jdsimple.h"

static jd_frame_t sendFrame[2];
static uint8_t bufferPtr, isSending;

int txq_is_idle() {
    return !isSending && sendFrame[bufferPtr].size == 0;
}

void *txq_push(unsigned service_num, unsigned service_cmd, const void *data,
               unsigned service_size) {
    void *trg = jd_push_in_frame(&sendFrame[bufferPtr], service_num, service_cmd, service_size);
    if (!trg) {
        DMESG("send overflow!");
        return NULL;
    }
    if (data)
        memcpy(trg, data, service_size);
    return trg;
}

jd_frame_t *app_pull_frame(void) {
    isSending = true;
    return &sendFrame[bufferPtr ^ 1];
}

void app_frame_sent(jd_frame_t *pkt) {
    isSending = false;
}

void txq_flush() {
    if (isSending)
        return;
    if (sendFrame[bufferPtr].size == 0)
        return;
    sendFrame[bufferPtr].device_identifier = device_id();
    jd_compute_crc(&sendFrame[bufferPtr]);
    bufferPtr ^= 1;
    jd_packet_ready();

    jd_reset_frame(&sendFrame[bufferPtr]);
}
