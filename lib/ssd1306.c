#include "lib.h"

#define ADDR 0x3D // or 0x3C

#define PX_ADDR(x, y) (x + (y >> 3) * OLED_WIDTH)
#define PX_MASK(x, y) (1 << ((y)&7))

#define SETCONTRAST 0x81
#define DISPLAYALLONRESUME 0xA4
#define DISPLAYALLON 0xA5
#define NORMALDISPLAY 0xA6
#define INVERTDISPLAY 0xA7
#define DISPLAYOFF 0xAE
#define DISPLAYON 0xAF
#define SETDISPLAYOFFSET 0xD3
#define SETCOMPINS 0xDA
#define SETVCOMDESELECT 0xDB
#define SETDISPLAYCLOCKDIV 0xD5
#define SETPRECHARGE 0xD9
#define SETMULTIPLEX 0xA8
#define SETLOWCOLUMN 0x00
#define SETHIGHCOLUMN 0x10
#define SETSTARTLINE 0x40
#define MEMORYMODE 0x20
#define COMSCANINC 0xC0
#define COMSCANDEC 0xC8
#define SEGREMAP 0xA0
#define CHARGEPUMP 0x8D
#define EXTERNALVCC 0x01
#define SWITCHCAPVCC 0x02

static const uint8_t init_cmds[] = {
    //
    DISPLAYOFF,
    SETDISPLAYCLOCKDIV,
    0x80,
    SETMULTIPLEX,
    0x2F,
    SETDISPLAYOFFSET,
    0x0,
    SETSTARTLINE | 0x0,
    CHARGEPUMP,
    0x14,
    NORMALDISPLAY,
    DISPLAYALLONRESUME,
    SEGREMAP | 0x1,
    COMSCANDEC,
    SETCOMPINS,
    0x12,
    SETCONTRAST,
    0x8F,
    SETPRECHARGE,
    0xF1,
    SETVCOMDESELECT,
    0x40,
    MEMORYMODE,
    0x00, // vertical
    DISPLAYON,
};

static void command(uint8_t cmd) {
    i2c_write_reg(ADDR, 0x00, cmd);
}

static void data(const void *ptr, unsigned len) {
    i2c_write_reg_buf(ADDR, 0x40, ptr, len);
}

#if 0
static void commands(const void *ptr, unsigned len) {
    i2c_write_reg_buf(ADDR, 0x00, ptr, len);
}
#endif

void oled_flush(oled_state_t *ctx) {
#if 0
    static const uint8_t setaddr[] = {
        0x22, 0, 0xff,
        0x21, 0, OLED_WIDTH - 1
    };
  commands(setaddr, sizeof(setaddr));
#endif
    for (unsigned i = 0; i < sizeof(ctx->databuf); i += OLED_WIDTH) {
        command(0xb0 | (i / OLED_WIDTH)); // pageaddr
        command(0x12);
        command(0x00);
        data(ctx->databuf + i, OLED_WIDTH);
    }
}

void oled_set_pixel(oled_state_t *ctx, int x, int y) {
    ctx->databuf[PX_ADDR(x, y)] |= PX_MASK(x, y);
}

void oled_setup(oled_state_t *ctx) {
    i2c_init();
    for (unsigned i = 0; i < sizeof(init_cmds); ++i)
        command(init_cmds[i]);
    for (unsigned i = 0; i < 64; ++i) {
        oled_set_pixel(ctx, i, i);
        oled_set_pixel(ctx, i, 63 - i);
    }
    // memset(ctx->databuf,0xee,sizeof(ctx->databuf));
    oled_flush(ctx);
}