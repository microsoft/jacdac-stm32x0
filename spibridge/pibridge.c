#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <linux/gpio.h>
#include <stdbool.h>

int spi_fd;
int rxfd;

#define SPI_DEV "/dev/spidev0.0"
#define GPIO_CHIP "/dev/gpiochip0"
#define CONSUMER "jacdac"
#define XFER_SIZE 256

void fatal(const char *msg) {
    fprintf(stderr, "Fatal error: %s (%s)\n", msg, strerror(errno));
    exit(1);
}

int req_lines(const unsigned *lines, unsigned numlines, unsigned flags) {
    struct gpio_v2_line_request req;

    memset(&req, 0, sizeof(req));

    req.config.flags = flags;
    strcpy(req.consumer, "jacdac-SPI");
    req.num_lines = numlines;
    for (unsigned i = 0; i < numlines; ++i)
        req.offsets[i] = lines[i];

    int gpio_fd = open(GPIO_CHIP, 0);
    if (gpio_fd < 0)
        fatal("can't open " GPIO_CHIP);

    int ret = ioctl(gpio_fd, GPIO_V2_GET_LINE_IOCTL, &req);
    if (ret == -1)
        fatal("can't GPIO_GET_LINE_IOCTL");

    close(gpio_fd);

    return req.fd;
}

void set_pin(int fd, int val) {
    struct gpio_v2_line_values vals;
    vals.bits = val ? 1 : 0;
    vals.mask = 1;

    int ret = ioctl(fd, GPIO_V2_LINE_SET_VALUES_IOCTL, &vals);
    if (ret == -1)
        fatal("failed GPIOHANDLE_SET_LINE_VALUES_IOCTL");
}

unsigned get_pins(int fd, int num) {
    struct gpio_v2_line_values vals;
    vals.bits = 0;
    vals.mask = (1 << num) - 1;

    int ret = ioctl(fd, GPIO_V2_LINE_GET_VALUES_IOCTL, &vals);
    if (ret == -1)
        fatal("failed GPIOHANDLE_SET_LINE_VALUES_IOCTL");

    return vals.bits;
}

#define PIN_TX_READY 24   // RST; G0 is ready for data from Pi
#define PIN_RX_READY 25   // AN; G0 has data for Pi
#define PIN_BRIDGE_RST 22 // nRST of the bridge G0 MCU

#define GPIOD_OK(v)                                                                                \
    if (0 != (v))                                                                                  \
        return 1;

struct timespec start_time;
uint8_t emptyqueue[XFER_SIZE];
uint8_t txqueue[XFER_SIZE + 4];
uint8_t rxqueue[XFER_SIZE];
int txq_ptr;
pthread_mutex_t sendmut;
pthread_cond_t txfree;
pthread_t detect_rxtx_ready_thread;

// https://wiki.nicksoft.info/mcu:pic16:crc-16:home
uint16_t jd_crc16(const void *data, uint32_t size) {
    const uint8_t *ptr = (const uint8_t *)data;
    uint16_t crc = 0xffff;
    while (size--) {
        uint8_t data = *ptr++;
        uint8_t x = (crc >> 8) ^ data;
        x ^= x >> 4;
        crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
    }
    return crc;
}

void delay(unsigned long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000ul;                // whole seconds
    ts.tv_nsec = (ms % 1000ul) * 1000000ul; // remainder, in nanoseconds
    nanosleep(&ts, NULL);
}

unsigned long millis() {
    struct timespec now_time;
    clock_gettime(CLOCK_MONOTONIC, &now_time);

    return (now_time.tv_sec - start_time.tv_sec) * 1000 +
           (now_time.tv_nsec - start_time.tv_nsec) / 1000000L;
}

void warning(const char *msg) {
    fprintf(stderr, "warning: %s\n", msg);
}

static void printpkt(int ts, const uint8_t *ptr, int len) {
    printf("%d ", ts);
    for (int i = 0; i < len; ++i) {
        printf("%02x", ptr[i]);
    }
    printf("\n");
}

void xfer();

static void queuetx(const uint8_t *ptr, int len) {
    if (len < 12) {
        warning("too short pkt TX");
        return;
    }
    uint16_t crc = jd_crc16(ptr + 2, len - 2);
    if ((ptr[0] | (ptr[1] << 8)) != crc) {
        warning("wrong crc TX");
        return;
    }

    pthread_mutex_lock(&sendmut);
    while (txq_ptr + len > XFER_SIZE)
        pthread_cond_wait(&txfree, &sendmut);
    memcpy(txqueue + txq_ptr, ptr, len);
    txq_ptr += (len + 3) & ~3;
    pthread_mutex_unlock(&sendmut);
    xfer();
}

void xfer() {
    pthread_mutex_lock(&sendmut);

    unsigned v = get_pins(rxfd, 2);
    int rx_value = v & 1;
    int tx_value = v & 2;

    int sendtx = txq_ptr && tx_value;
    if (rx_value || sendtx) {
        struct spi_ioc_transfer spi;
        if (sendtx)
            memset(txqueue + txq_ptr, 0, 4);
        memset(rxqueue, 0, 4);
        memset(&spi, 0, sizeof(spi));
        spi.tx_buf = sendtx ? (unsigned long)txqueue : (unsigned long)emptyqueue;
        int timestamp = millis();
        spi.rx_buf = (unsigned long)rxqueue;
        spi.len = XFER_SIZE;
        spi.delay_usecs = 0;
        spi.speed_hz = 16 * 1000 * 1000;
        spi.bits_per_word = 8;
        int ok = 0;
        for (int i = 0; i < 10; ++i) {
            int r = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &spi);
            if (r < 0) {
                fprintf(stderr, "err=%d %s %d\n", r, strerror(errno), spi_fd);
                warning("ioctl SPI failed");
                delay(1);
            } else {
                ok = 1;
                break;
            }
        }
        if (!ok)
            fatal("IOCTL SPI error");
        uint8_t *frame = rxqueue;
        while (frame < rxqueue + XFER_SIZE) {
            if (frame[2] == 0)
                break;
            int sz = frame[2] + 12;
            if (frame + sz > rxqueue + XFER_SIZE) {
                warning("packet overflow");
                break;
            }
            if (frame[0] == 0xff && frame[1] == 0xff && frame[3] == 0xff) {
                // skip bogus packet
            } else {
                uint16_t crc = jd_crc16(frame + 2, sz - 2);
                if (*(uint16_t *)frame != crc)
                    warning("invalid crc");
                printpkt(timestamp, frame, sz);
            }
            sz = (sz + 3) & ~3;
            frame += sz;
        }
        fflush(stdout);
        if (sendtx) {
            txq_ptr = 0;
            pthread_cond_signal(&txfree);
        }
    }
    pthread_mutex_unlock(&sendmut);
}

int hexdig(char c) {
    if ('0' <= c && c <= '9')
        return c - '0';
    c |= 0x20;
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    return -1;
}

void *detect_rxtx_ready() {
    struct gpio_v2_line_event event;
    while (true) {
        int ret = read(rxfd, &event, sizeof(event));
        if (ret == -1) {
            if (errno == -EAGAIN) {
                // fprintf(stderr, "nothing available\n");
                continue;
            } else {
                fatal("failed to read event");
            }
        }

        if (ret != sizeof(event))
            fatal("short event read");
        xfer();
    }
    return NULL;
}

int work(void) {
    unsigned int rxtx_offsets[] = {PIN_RX_READY, PIN_TX_READY};
    unsigned int rst_offsets[] = {PIN_BRIDGE_RST};

    int rstfd = req_lines(rst_offsets, 1, GPIO_V2_LINE_FLAG_OUTPUT);
    set_pin(rstfd, 0);
    delay(10);
    set_pin(rstfd, 1);
    close(rstfd);
    // re-request as input and release; needed?
    rstfd = req_lines(rst_offsets, 1, GPIO_V2_LINE_FLAG_INPUT);
    close(rstfd);

    rxfd = req_lines(rxtx_offsets, 2,
                     GPIO_V2_LINE_FLAG_INPUT | GPIO_V2_LINE_FLAG_EDGE_RISING |
                         GPIO_V2_LINE_FLAG_BIAS_PULL_DOWN);

    pthread_mutex_init(&sendmut, NULL);
    pthread_cond_init(&txfree, NULL);
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    spi_fd = open(SPI_DEV, O_RDWR);
    if (spi_fd < 0) {
        printf("error opening %s: %s\n", SPI_DEV, strerror(errno));
        return 1;
    }

    // uint8_t spiMode = SPI_NO_CS;
    uint8_t spiMode = 0;
    int r = ioctl(spi_fd, SPI_IOC_WR_MODE, &spiMode);
    if (r != 0)
        fatal("can't WR_MODE on SPI");

    pthread_create(&detect_rxtx_ready_thread, NULL, detect_rxtx_ready, NULL);

    xfer();

    while (true) {
        uint8_t pkt[XFER_SIZE];
        int ptr = 0;
        for (;;) {
            int c = fgetc(stdin);
            if (c < 0)
                return 0;
            if (c == '\r' || c == '\n')
                break;
            if (c == ' ' || c == '\t')
                continue;
            int high = hexdig(c);
            if (high == -1) {
                warning("invalid character");
                continue;
            }
            int low = hexdig(fgetc(stdin));
            if (low == -1) {
                warning("invalid character #2");
                continue;
            }
            pkt[ptr++] = low | (high << 4);
            if (ptr >= XFER_SIZE - 4) {
                warning("too large pkt");
                break;
            }
        }
        if (ptr)
            queuetx(pkt, ptr);
    }
    return 0;
}

int main(void) {
    return work();
}