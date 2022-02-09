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
#include <gpiod.h> // https://git.kernel.org/pub/scm/libs/libgpiod/libgpiod.git/

int spi_fd;

#define SPI_DEV "/dev/spidev0.0"
#define GPIO_CHIP "/dev/gpiochip0"
#define CONSUMER "jacdac"
#define XFER_SIZE 256

#define PIN_TX_READY 24   // RST; G0 is ready for data from Pi
#define PIN_RX_READY 25   // AN; G0 has data for Pi
#define PIN_BRIDGE_RST 22 // nRST of the bridge G0 MCU

#define GPIOD_OK(v) if (0 != (v)) return 1;

volatile sig_atomic_t sigint_received = 0;
struct gpiod_chip *chip;
struct gpiod_line_bulk rxtx_lines;
struct gpiod_line_bulk rxtx_events;
struct timespec start_time;
uint8_t emptyqueue[XFER_SIZE];
uint8_t txqueue[XFER_SIZE + 4];
uint8_t rxqueue[XFER_SIZE];
int txq_ptr;
pthread_mutex_t sendmut;
pthread_cond_t txfree;
pthread_t detect_rxtx_ready_thread;
volatile bool detecting_rxtx;

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

void delay(unsigned long ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000ul;            // whole seconds
    ts.tv_nsec = (ms % 1000ul) * 1000000ul;  // remainder, in nanoseconds
    nanosleep(&ts, NULL);
}

unsigned long millis() {
    struct timespec now_time;
    clock_gettime(CLOCK_MONOTONIC, &now_time);

    return (now_time.tv_sec - start_time.tv_sec) * 1000 + (now_time.tv_nsec - start_time.tv_nsec) / 1000000L;
}

void warning(const char *msg) { fprintf(stderr, "warning: %s\n", msg); }
void fatal(const char *msg) {
  fprintf(stderr, "fatal error: %s\n", msg);
  exit(3);
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
  fprintf(stderr, "xfer\n");
  pthread_mutex_lock(&sendmut);

  int rxtx[2];
  gpiod_line_get_value_bulk(&rxtx_lines, rxtx);
  int rx_value = rxtx[0];
  int tx_value = rxtx[1];
  fprintf(stderr, "rx %d, tx %d\n", rx_value, tx_value);

  int sendtx = txq_ptr && tx_value;
  if (rx_value || sendtx) {
    fprintf(stderr, "ioctl\n");
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
  fprintf(stderr, "xfer done\n");
}

int hexdig(char c) {
  if ('0' <= c && c <= '9')
    return c - '0';
  c |= 0x20;
  if ('a' <= c && c <= 'f')
    return c - 'a' + 10;
  return -1;
}

void* detect_rxtx_ready(){
  fprintf(stderr, "starting edge detection\n");
  while (detecting_rxtx && !sigint_received) {
    fprintf(stderr, "wait for edges\n");
    struct gpiod_line_bulk rxtx_events;
    int ret = gpiod_line_event_wait_bulk(&rxtx_lines, NULL, &rxtx_events);
    fprintf(stderr, "edge detect: %d\n", ret);
    xfer();
  }
  fprintf(stderr, "end edge detection\n");
  return NULL;
}

void sigint_handler() {
  sigint_received = 1;
}

void cleanup(void) {
  fprintf(stderr, "starting transfer\n");
  if (detecting_rxtx) {
    printf("cancelling spi edge thread\n");
    detecting_rxtx = false;
    pthread_join(detect_rxtx_ready_thread, NULL);
  }
  if (NULL != chip) {
    printf("closing chip\n");
    gpiod_line_release_bulk(&rxtx_lines);
    gpiod_chip_close(chip);
  }
}

int work(void) {

  unsigned int rxtx_offsets[] = { PIN_RX_READY, PIN_TX_READY };
 
  pthread_mutex_init(&sendmut, NULL);
  pthread_cond_init(&txfree, NULL);
  clock_gettime(CLOCK_MONOTONIC, &start_time);  

  // request rx, tx
  chip = gpiod_chip_open(GPIO_CHIP);
  if (NULL == chip) {
    printf("error opening gpio chip %s\n", GPIO_CHIP);
    return 1;
  }

  fprintf(stderr, "opening rx, tx\n");
  GPIOD_OK(gpiod_chip_get_lines(chip, rxtx_offsets, 2, &rxtx_lines));
  fprintf(stderr, "requesting rx, tx edge events\n");
  GPIOD_OK(gpiod_line_request_bulk_rising_edge_events_flags(&rxtx_lines, CONSUMER, GPIOD_LINE_BIAS_PULL_DOWN));

  // reset spi bridge
  fprintf(stderr, "reseting bridge\n");
  {
    struct gpiod_line *rst = gpiod_chip_get_line(chip, PIN_BRIDGE_RST);
    GPIOD_OK(gpiod_line_request_output(rst, CONSUMER, 0));
    GPIOD_OK(gpiod_line_set_value(rst, 0));
    delay(100);
    GPIOD_OK(gpiod_line_set_value(rst, 1));
    GPIOD_OK(gpiod_line_set_direction_input(rst));
    gpiod_line_release(rst);
  }

  fprintf(stderr, "starting spi\n");
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

  fprintf(stderr, "starting edge detection thread\n");
  detecting_rxtx = true;
  pthread_create(&detect_rxtx_ready_thread, NULL, detect_rxtx_ready, NULL);

  fprintf(stderr, "starting transfer\n");
  xfer();

  while (!sigint_received) {
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
    signal(SIGINT, sigint_handler);
    int res = work();
    cleanup();
    return res;
}