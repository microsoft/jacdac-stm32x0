#include "jdsimple.h"

//#define LOG DMESG
#define LOG(...) ((void)0)

bool should_sample(uint32_t *sample, uint32_t period) {
    if (in_future(*sample))
        return false;

    *sample += period;

    if (!in_future(*sample))
        // we lost some samples
        *sample = now + period;

    return true;
}

REG_DEFINITION(                         //
    sensor_regs,                        //
    REG_BIT(JD_REG_IS_STREAMING),       //
    REG_U8(JD_REG_PADDING),             // service_number not accessible
    REG_U32(JD_REG_STREAMING_INTERVAL), //
    REG_U32(JD_REG_PADDING),            // next_sample not accesible
);

int sensor_handle_packet(sensor_state_t *state, jd_packet_t *pkt) {
    int r = handle_reg(state, pkt, sensor_regs);
    switch (r) {
    case JD_REG_IS_STREAMING:
        if (state->is_streaming) {
            if (state->streaming_interval == 0)
                state->streaming_interval = 100;
            state->next_sample = now;
        }
        break;
    case JD_REG_STREAMING_INTERVAL:
        if (state->streaming_interval < 20)
            state->streaming_interval = 20;
        if (state->streaming_interval > 100000)
            state->streaming_interval = 100000;
        break;
    }
    return r;
}

int sensor_should_stream(sensor_state_t *state) {
    if (!state->is_streaming)
        return false;
    return should_sample(&state->next_sample, state->streaming_interval * 1000);
}

#define REG_IS_SIGNED(r) ((r) <= 4 && !((r)&1))
static const uint8_t regSize[16] = {1, 1, 2, 2, 4, 4, 4, 8, 1};

int handle_reg(void *state, jd_packet_t *pkt, const uint16_t sdesc[]) {
    bool is_get = (pkt->service_command >> 12) == (JD_CMD_GET_REG >> 12);
    bool is_set = (pkt->service_command >> 12) == (JD_CMD_SET_REG >> 12);
    if (!is_get && !is_set)
        return 0;

    if (is_set && pkt->service_size == 0)
        return 0;

    int reg = pkt->service_command & 0xfff;

    if (reg >= 0xf00) // these are reserved
        return 0;

    if (is_set && (reg & 0xf00) == 0x100)
        return 0; // these are read-only

    uint32_t offset = 0;
    uint8_t bitoffset = 0;

    LOG("handle %x", reg);

    for (int i = 0; sdesc[i] != JD_REG_END; ++i) {
        uint16_t sd = sdesc[i];
        int tp = sd >> 12;
        int regsz = regSize[tp];

        if (tp == _REG_BYTES)
            regsz = sdesc[++i];

        if (!regsz)
            jd_panic();

        if (tp != _REG_BIT) {
            if (bitoffset) {
                bitoffset = 0;
                offset++;
            }
            int align = regsz < 4 ? regsz - 1 : 3;
            offset = (offset + align) & ~align;
        }

        LOG("%x:%d:%d",(sd & 0xfff),offset,regsz);

        if ((sd & 0xfff) == reg) {
            uint8_t *sptr = (uint8_t *)state + offset;
            if (is_get) {
                if (tp == _REG_BIT) {
                    uint8_t v = *sptr & (1 << bitoffset) ? 1 : 0;
                    txq_push(pkt->service_number, pkt->service_command, &v, 1);
                } else {
                    txq_push(pkt->service_number, pkt->service_command, sptr, regSize[tp]);
                }
                return -reg;
            } else {
                if (tp == _REG_BIT) {
                    LOG("bit @%d - %x", offset, reg);
                    if (pkt->data[0])
                        *sptr |= 1 << bitoffset;
                    else
                        *sptr &= ~(1 << bitoffset);
                } else if (regsz <= pkt->service_size) {
                    LOG("exact @%d - %x", offset, reg);
                    memcpy(sptr, pkt->data, regsz);
                } else {
                    LOG("too little @%d - %x", offset, reg);
                    memcpy(sptr, pkt->data, pkt->service_size);
                    int fill = !REG_IS_SIGNED(tp)
                                   ? 0
                                   : (pkt->data[pkt->service_size - 1] & 0x80) ? 0xff : 0;
                    memset(sptr + pkt->service_size, fill, regsz - pkt->service_size);
                }
                return reg;
            }
        }

        if (tp == _REG_BIT) {
            bitoffset++;
            if (bitoffset == 8) {
                offset++;
                bitoffset = 0;
            }
        } else {
            offset += regsz;
        }
    }

    return 0;
}
