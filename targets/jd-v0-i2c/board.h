#define PIN_LED PA_1
#define PIN_LED_GND PA_0
#define PIN_LED2 -1
#define PIN_LOG0 -1 // sig_write
#define PIN_LOG1 -1
#define PIN_LOG2 -1 // sig error
#define PIN_LOG3 -1

#define PIN_PWR PA_5
#define PIN_P0 -1
#define PIN_P1 -1

#define PIN_SERVO -1

#define UART_PIN PA_2
#define UART_PIN_AF LL_GPIO_AF_1
#define USART_IDX 1

#define PIN_SDA PA_10
#define PIN_SCL PA_9
#define I2C_FAST_MODE 0

#define OUTPUT_PINS                                                                                \
    PIN_LOG0, PIN_LOG1, PIN_LOG2, PIN_LOG3, PIN_LED, PIN_LED2, PIN_PWR, PIN_P0, PIN_P1,            \
        PIN_LED_GND, PIN_SERVO,
