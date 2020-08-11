#define PIN_LED PB_1
#define PIN_LED_GND -1
#define PIN_LED2 -1
#define PIN_LOG0 -1 // sig_write
#define PIN_LOG1 -1
#define PIN_LOG2 -1 // sig error
#define PIN_LOG3 -1

#define PIN_PWR -1
#define PIN_P0 PF_0
#define PIN_P1 PF_1

#define PIN_SDA PA_10
#define PIN_SCL PA_9
#define I2C_AF LL_GPIO_AF_4

#define PIN_ACC_INT PA_7

#define UART_PIN PA_2
#define UART_PIN_AF LL_GPIO_AF_1
#define USART_IDX 1

#define PWR_PIN_PULLUP 1

#define OUTPUT_PINS                                                                                \
    PIN_LOG0, PIN_LOG1, PIN_LOG2, PIN_LOG3, PIN_LED, PIN_LED2, PIN_P0, PIN_P1, PIN_LED_GND

#define PIN_ASCK PA_5 // yellow
#define PIN_AMOSI PA_7 // red
#define PIN_AMISO PA_6 // orng
// CS PA_3  // blue
// TXRQ PA_4 // grn 
#define SPI_RX 1

struct _jd_frame_t;
void bridge_forward_frame(struct _jd_frame_t *frame);
#define JD_SERVICES_PROCESS_FRAME_PRE(fr) bridge_forward_frame(fr)
