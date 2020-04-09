#include "jdsimple.h"

#define MASK_SET(p) (1 << ((p)&0xf))
#define MASK_CLR(p) (1 << (((p)&0xf) + 16))

#define REG_CHIP_ID 0x00
#define REG_DX 0x01
#define REG_DY 0x03
#define REG_DZ 0x05
#define REG_STEP_CNT0 0x07
#define REG_STEP_CNT1 0x08
#define REG_INT_ST0 0x09
#define REG_INT_ST1 0x0a
#define REG_INT_ST2 0x0b
#define REG_STEP_CNT2 0x0e
#define REG_FSR 0x0f
#define REG_BW 0x10
#define REG_PM 0x11
#define REG_STEP_CONF0 0x12
#define REG_STEP_CONF1 0x13
#define REG_STEP_CONF2 0x14
#define REG_STEP_CONF3 0x15
#define REG_INT_EN0 0x16
#define REG_INT_EN1 0x17
#define REG_INT_EN2 0x18
#define REG_INT_MAP0 0x19
#define REG_INT_MAP1 0x1a
#define REG_INT_MAP2 0x1b
#define REG_INT_MAP3 0x1c
#define REG_SIG_STEP_TH 0x1d
#define REG_INTPIN_CONF 0x20
#define REG_INT_CFG 0x21
#define REG_OS_CUST_X 0x27
#define REG_OS_CUST_Y 0x28
#define REG_OS_CUST_Z 0x29
#define REG_MOT_CONF0 0x2c
#define REG_MOT_CONF1 0x2d
#define REG_MOT_CONF2 0x2e
#define REG_MOT_CONF3 0x2f
#define REG_ST 0x32
#define REG_RAISE_WAKE_PERIOD 0x35
#define REG_SR 0x36
#define REG_RAISE_WAKE_TIMEOUT_TH 0x3e

#define QMAX981_RANGE_2G 0x01
#define QMAX981_RANGE_4G 0x02
#define QMAX981_RANGE_8G 0x04
#define QMAX981_RANGE_16G 0x08
#define QMAX981_RANGE_32G 0x0f

#define QMAX981_BW_8HZ 0xE7
#define QMAX981_BW_16HZ 0xE6
#define QMAX981_BW_32HZ 0xE5
#define QMAX981_BW_65HZ 0xE0
#define QMAX981_BW_130HZ 0xE1
#define QMAX981_BW_258HZ 0xE2
#define QMAX981_BW_513HZ 0xE3

// 1 is around 1MHz - still works up to 1.25MHz or so
// 5 is 660kHz
// 10 is 430kHz
#define NOP target_wait_cycles(5)

#define SEND_BIT(n)                                                                                \
    NOP;                                                                                           \
    ACC_PORT->BSRR = MASK_CLR(PIN_ACC_MOSI) | (((b >> n) & 1) << (PIN_ACC_MOSI & 0xf));            \
    ACC_PORT->BSRR = MASK_SET(PIN_ACC_SCK);                                                        \
    NOP;                                                                                           \
    ACC_PORT->BSRR = MASK_CLR(PIN_ACC_SCK)

#define RECV_BIT(n)                                                                                \
    NOP;                                                                                           \
    ACC_PORT->BSRR = MASK_SET(PIN_ACC_SCK);                                                        \
    NOP;                                                                                           \
    b |= ((ACC_PORT->IDR >> (PIN_ACC_MISO & 0xf)) & 1) << n;                                       \
    ACC_PORT->BSRR = MASK_CLR(PIN_ACC_SCK)

static void send(const uint8_t *src, uint32_t len) {
    // target_wait_us(5);
    for (int i = 0; i < len; ++i) {
        uint8_t b = src[i];
        SEND_BIT(7);
        SEND_BIT(6);
        SEND_BIT(5);
        SEND_BIT(4);
        SEND_BIT(3);
        SEND_BIT(2);
        SEND_BIT(1);
        SEND_BIT(0);
    }
    pin_set(PIN_ACC_MOSI, 0);
}

static void recv(uint8_t *dst, uint32_t len) {
    // target_wait_us(5);
    for (int i = 0; i < len; ++i) {
        uint8_t b = 0x00;
        RECV_BIT(7);
        RECV_BIT(6);
        RECV_BIT(5);
        RECV_BIT(4);
        RECV_BIT(3);
        RECV_BIT(2);
        RECV_BIT(1);
        RECV_BIT(0);
        dst[i] = b;
    }
}

static void writeReg(uint8_t reg, uint8_t val) {
    // target_wait_us(200);
    uint8_t cmd[] = {
        0xAE, // IDW
        0x00, // reg high
        reg,  // register
        0x00, // len high
        0x01, // len low,
        val,  // data
    };
    pin_set(PIN_ACC_CS, 0);
    send(cmd, sizeof(cmd));
    pin_set(PIN_ACC_CS, 1);
}

static void readData(uint8_t reg, uint8_t *dst, int len) {
    pin_set(PIN_ACC_CS, 0);
    uint8_t cmd[] = {
        0xAF, // IDR
        0x00, // reg high
        reg,  // register
        0x00, // len high
        len,  // len low,
    };

    send(cmd, sizeof(cmd));
    recv(dst, len);
    pin_set(PIN_ACC_CS, 1);
}

static int readReg(uint8_t reg) {
    uint8_t r = 0;
    readData(reg, &r, 1);
    return r;
}

static void init_chip() {
    writeReg(REG_PM, 0x80);
    writeReg(REG_SR, 0xB6);
    target_wait_us(100);
    writeReg(REG_SR, 0x00);
    writeReg(REG_PM, 0x80);
    writeReg(REG_INT_CFG, 0x1C | (1 << 5)); // disable I2C
    writeReg(REG_INTPIN_CONF, 0x05 | (1 << 7)); // disable pull-up on CS
    writeReg(REG_FSR, QMAX981_RANGE_8G);

    // ~50uA
    // writeReg(REG_PM, 0x84); // MCK 50kHz
    // writeReg(REG_BW, 0xE3); // sample 50kHz/975 = 51.3Hz

    // ~60uA
    writeReg(REG_PM, 0x83); // MCK 100kHz
    writeReg(REG_BW, 0xE2); // sample 100kHz/1935 = 51.7Hz

//    writeReg(REG_PM, 0x40); // sleep

    // ~150uA - 500kHz
    // writeReg(REG_BW, 0xE0); // 65Hz
    // 0xE1 130Hz
    // 0xE2 260Hz
}

void acc_hw_get(int16_t sample[3]) {
    int16_t data[3];
    pin_setup_input(PIN_ACC_MISO, -1);
    readData(REG_DX, (uint8_t *)data, 6);
    pin_setup_analog_input(PIN_ACC_MISO);
    sample[0] = data[1] >> 2;
    sample[1] = -data[0] >> 2;
    sample[2] = -data[2] >> 2;
}

void acc_hw_init() {
    pin_setup_output(PIN_ACC_MOSI);
    pin_setup_output(PIN_ACC_SCK);
    pin_setup_input(PIN_ACC_MISO, -1);

    pin_setup_output(PIN_ACC_VCC);
    pin_setup_output(PIN_ACC_CS);

    pin_set(PIN_ACC_CS, 1);
    pin_set(PIN_ACC_VCC, 1);

    // 9us is enough; datasheet claims 250us for I2C ready and 2ms for conversion ready
    target_wait_us(250);

    int v = readReg(REG_CHIP_ID);
    DMESG("acc id: %x", v);

    if (0xe0 <= v && v <= 0xe9) {
        if (v >= 0xe8) {
            // v2
        }
    } else {
        DMESG("invalid chip");
        jd_panic();
    }

    init_chip();

    pin_setup_analog_input(PIN_ACC_MISO);
}
