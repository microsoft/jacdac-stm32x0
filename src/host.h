#pragma once

#define PKT_UNHANDLED 0
#define PKT_HANDLED_RO 1
#define PKT_HANDLED_RW 2

#define JD_REG_PADDING 0xff0
#define JD_REG_END 0xff1
#define _REG_(tp, v) (((tp) << 12) | (v))
#define _REG_S8 0
#define _REG_U8 1
#define _REG_S16 2
#define _REG_U16 3
#define _REG_S32 4
#define _REG_U32 5
#define _REG_BYTE4 6
#define _REG_BYTE8 7
#define _REG_BIT 8
#define _REG_BYTES 9
#define REG_S8(v) _REG_(_REG_S8, (v))
#define REG_U8(v) _REG_(_REG_U8, (v))
#define REG_S16(v) _REG_(_REG_S16, (v))
#define REG_U16(v) _REG_(_REG_U16, (v))
#define REG_S32(v) _REG_(_REG_S32, (v))
#define REG_U32(v) _REG_(_REG_U32, (v))
#define REG_BYTE4(v) _REG_(_REG_BYTE4, (v))
#define REG_BYTE8(v) _REG_(_REG_BYTE8, (v))
#define REG_BIT(v) _REG_(_REG_BIT, (v))
#define REG_BYTES(v, n) _REG_(_REG_BYTES, (v)), n

#define REG_DEFINITION(name, ...) static const uint16_t name[] = {__VA_ARGS__ JD_REG_END};

int handle_reg(void *state, jd_packet_t *pkt, const uint16_t sdesc[]);

// keep sampling at period, using state at *sample
bool should_sample(uint32_t *sample, uint32_t period);

// sensor helpers
struct _sensor_state {
    uint8_t is_streaming : 1;
    uint8_t service_number;
    uint32_t streaming_interval;
    uint32_t next_sample;
};
typedef struct _sensor_state sensor_state_t;

int sensor_handle_packet(sensor_state_t *state, jd_packet_t *pkt);
int sensor_should_stream(sensor_state_t *state);

typedef void (*pkt_handler_t)(jd_packet_t *pkt);
typedef void (*service_fn_t)(uint8_t service_num);

struct _host_service {
    uint32_t service_class;
    service_fn_t init;
    cb_t process;
    pkt_handler_t handle_pkt;
};
typedef struct _host_service host_service_t;

extern const host_service_t host_ctrl;
extern const host_service_t host_accelerometer;
extern const host_service_t host_light;
extern const host_service_t host_pwm_light;
extern const host_service_t host_crank;