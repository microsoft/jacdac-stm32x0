#include "jdsimple.h"

#define JD_ARCADE_CONTROLS_BUTTON_LEFT 0x0001
#define JD_ARCADE_CONTROLS_BUTTON_UP 0x0002
#define JD_ARCADE_CONTROLS_BUTTON_RIGHT 0x0003
#define JD_ARCADE_CONTROLS_BUTTON_DOWN 0x0004
#define JD_ARCADE_CONTROLS_BUTTON_A 0x0005
#define JD_ARCADE_CONTROLS_BUTTON_B 0x0006
#define JD_ARCADE_CONTROLS_BUTTON_MENU 0x0007
#define JD_ARCADE_CONTROLS_BUTTON_MENU2 0x0008
#define JD_ARCADE_CONTROLS_BUTTON_RESET 0x0009
#define JD_ARCADE_CONTROLS_BUTTON_EXIT 0x000a

typedef struct {
    uint8_t flags;
    uint8_t numplayers;
    uint16_t buttons[0];
} jd_arcade_controls_advertisement_data_t;

typedef struct {
    uint16_t button;
    uint8_t player_index;
    uint8_t pressure; // for analog joysticks mostly, for digital inputs should be 0xff
} jd_arcade_controls_report_entry_t;

typedef struct {
    jd_arcade_controls_report_entry_t pressedButtons[0];
} jd_arcade_controls_report_t;


static const uint8_t buttonPins[] = {GAMEPAD_BUTTONS};

#define EVT_DOWN 1
#define EVT_UP 2
// these two below not implemented yet
#define EVT_CLICK 3
#define EVT_LONG_CLICK 4 

#define BUTTON_FLAGS 0
#define NUM_PLAYERS 1

#define NUM_PINS (int)sizeof(buttonPins)

struct srv_state {
    SENSOR_COMMON;
    uint8_t inited;
    uint32_t btn_state;

    uint8_t pin;
    uint8_t pressed;
    uint8_t prev_pressed;
    uint8_t num_zero;
    uint32_t press_time;
    uint32_t nextSample;
};

static uint32_t buttons_state(srv_t *state) {
    if (!state->inited) {
        state->inited = true;
        for (int i = 0; i < NUM_PINS; ++i) {
            pin_setup_input(buttonPins[i], 1);
        }
    }

    uint32_t r = 0;

    for (int i = 0; i < NUM_PINS; ++i) {
        if (pin_get(buttonPins[i]) == 0) {
            r |= (1 << i);
        }
    }

    return r;
}

static void update(srv_t *state) {
    uint32_t newstate = buttons_state(state);
    if (newstate != state->btn_state) {
        for (int i = 0; i < NUM_PINS; ++i) {
            uint32_t isPressed = (newstate & (1 << i));
            uint32_t wasPressed = (state->btn_state & (1 << i));
            if (isPressed != wasPressed) {
                txq_push_event_ex(state, isPressed ? EVT_DOWN : EVT_UP, i + 1);
            }
        }
        state->btn_state = newstate;
    }
}

static void send_report(srv_t *state) {
    jd_arcade_controls_report_entry_t reports[NUM_PINS], *report;
    report = reports;

    for (int i = 0; i < NUM_PINS; ++i) {
        if (state->btn_state & (1 << i)) {
            report->button = i + 1;
            report->player_index = 0;
            report->pressure = 0xff;
            report++;
        }
    }

    txq_push(state->service_number, JD_CMD_GET_REG | JD_REG_READING, reports,
             (uint8_t *)report - (uint8_t *)reports);
}

static void ad_data(srv_t *state) {
    uint16_t addata[NUM_PINS + 1];
    uint16_t *dst = addata;
    *dst++ = BUTTON_FLAGS | (NUM_PLAYERS << 8);
    for (int i = 0; i < NUM_PINS; ++i) {
        if (buttonPins[i] != 0xff) {
            *dst++ = i + 1;
        }
    }
    txq_push(state->service_number, JD_CMD_ADVERTISEMENT_DATA, addata,
             (uint8_t *)dst - (uint8_t *)addata);
}

void gamepad_process(srv_t *state) {
    if (should_sample(&state->nextSample, 9000)) {
        update(state);

        if (sensor_should_stream(state) && (state->btn_state || state->num_zero < 20)) {
            state->num_zero++;
            send_report(state);
        }
    }
}

void gamepad_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet(state, pkt);

    if (pkt->service_command == (JD_CMD_GET_REG | JD_REG_READING))
        send_report(state);
    else if (pkt->service_command == JD_CMD_ADVERTISEMENT_DATA)
        ad_data(state);
}

SRV_DEF(gamepad, JD_SERVICE_CLASS_ARCADE_CONTROLS);

void gamepad_init(void) {
    SRV_ALLOC(gamepad);
    update(state);
}
