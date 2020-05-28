#define PIN_LED PA_1
#define PIN_LED_GND PA_0
#define PIN_LED2 -1
#define PIN_LOG0 -1 // sig_write
#define PIN_LOG1 -1
#define PIN_LOG2 -1 // sig error
#define PIN_LOG3 -1

#define PIN_PWR PA_10
#define PIN_P0 PF_0
#define PIN_P1 PF_1

#define PIN_SERVO PA_7
#define PIN_AMOSI PA_7
#define PIN_ASCK PA_5

#define UART_PIN PA_9
#define UART_PIN_AF LL_GPIO_AF_1
#define USART_IDX 1

#define PWR_PIN_PULLUP 1

#define OUTPUT_PINS                                                                                \
    PIN_LOG0, PIN_LOG1, PIN_LOG2, PIN_LOG3, PIN_LED, PIN_LED2, PIN_P0, PIN_P1, PIN_LED_GND,        \
        PIN_AMOSI, PIN_ASCK,
