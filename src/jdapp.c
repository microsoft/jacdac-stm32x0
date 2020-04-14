#include "jdsimple.h"

#ifndef BL

#define MAX_SERV 32

static srv_t **services;
static uint8_t num_services;

static uint64_t maxId;
static uint32_t lastMax, lastDisconnectBlink;

#define ADD_SRV(name)                                                                              \
    extern void name##_init(void);                                                                 \
    name##_init();

#ifndef INIT_SERVICES
static inline void init_services() {
    // DMESG 1.1k

    //ADD_SRV(acc); // 2k
    //ADD_SRV(light); // 2.5k
    //ADD_SRV(crank); // 1k
    //ADD_SRV(pwm_light); // 2.5k
    //ADD_SRV(servo); // 2.5k
}
#define INIT_SERVICES() init_services()
#endif

struct srv_state {
    SRV_COMMON;
};

static int alloc_hash(const srv_vt_t *vt) {
    uint16_t hash = vt->service_class & 0xffff;
    if (!hash)
        return 0;
    uint16_t *hashes = (uint16_t *)services[MAX_SERV];
    int numcol = 1;
    int pos0 = -1;
    while (numcol) {
        numcol = 0;
        for (int i = 0; i < MAX_SERV; ++i) {
            if (hashes[i] == 0) {
                pos0 = i;
                break;
            }
            if (hashes[i] == hash) {
                hash++;
                numcol++;
            }
        }
    }
    if (pos0 < 0)
        jd_panic();
    hashes[pos0] = hash;
    return hash - (vt->service_class & 0xffff);
}

srv_t *srv_alloc(const srv_vt_t *vt) {
    // always allocate instances idx - it should be stable when we disable some services
    srv_t tmp;
    tmp.vt = vt;
    tmp.service_number = num_services;
    tmp.instance_idx = alloc_hash(vt);

    {
        if (settings_get_reg(&tmp, JD_REG_SERVICE_DISABLED) == 1)
            return NULL;
    }

    if (num_services >= MAX_SERV)
        jd_panic();
    srv_t *r = alloc(vt->state_size);
    *r = tmp;
    services[num_services++] = r;

    return r;
}

void app_init_services() {
    srv_t *tmp[MAX_SERV + 1];
    uint16_t hashes[MAX_SERV];
    tmp[MAX_SERV] = (srv_t *)hashes; // avoid global variable
    services = tmp;
    ADD_SRV(ctrl);
    INIT_SERVICES();
    services = alloc(sizeof(void *) * num_services);
    memcpy(services, tmp, sizeof(void *) * num_services);
}

void app_queue_annouce() {
    alloc_stack_check();

    uint32_t *dst =
        txq_push(JD_SERVICE_NUMBER_CTRL, JD_CMD_ADVERTISEMENT_DATA, NULL, num_services * 4);
    if (!dst)
        return;
    for (int i = 0; i < num_services; ++i)
        dst[i] = services[i]->vt->service_class;

#ifdef JDM_V2
    pin_setup_output(PIN_P0);
    pin_setup_output(PIN_SERVO);
    pin_set(PIN_P0, 1);
    pin_pulse(PIN_SERVO, 1);
    pin_pulse(PIN_PWR, 1);
    pin_pulse(PIN_SERVO, 1);
    pin_set(PIN_P0, 0);
#endif
}

static void handle_ctrl_tick(jd_packet_t *pkt) {
    if (pkt->service_command == JD_CMD_ADVERTISEMENT_DATA) {
        // if we have not seen maxId for 1.1s, find a new maxId
        if (pkt->device_identifier < maxId && in_past(lastMax + 1100000)) {
            maxId = pkt->device_identifier;
        }

        // maxId? blink!
        if (pkt->device_identifier >= maxId) {
            maxId = pkt->device_identifier;
            lastMax = now;
            led_blink(50);
        }
    }
}

void app_handle_packet(jd_packet_t *pkt) {
    if (!(pkt->flags & JD_FRAME_FLAG_COMMAND)) {
        if (pkt->service_number == 0)
            handle_ctrl_tick(pkt);
        return;
    }

    bool matched_devid = pkt->device_identifier == device_id();

    if (pkt->flags & JD_FRAME_FLAG_IDENTIFIER_IS_SERVICE_CLASS) {
        for (int i = 0; i < num_services; ++i) {
            if (pkt->device_identifier == services[i]->vt->service_class) {
                pkt->service_number = i;
                matched_devid = true;
                break;
            }
        }
    }

    if (!matched_devid)
        return;

    if (pkt->service_number < num_services) {
        srv_t *s = services[pkt->service_number];
        s->vt->handle_pkt(s, pkt);
    }
}

void app_process() {
    app_process_frame();

    if (should_sample(&lastDisconnectBlink, 250000)) {
        if (in_past(lastMax + 2000000)) {
            led_blink(5000);
        }
    }

    for (int i = 0; i < num_services; ++i) {
        services[i]->vt->process(services[i]);
    }

    txq_flush();
}

#endif