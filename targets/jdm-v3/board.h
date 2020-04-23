#define PIN_LED PA_15
#define PIN_LED_GND PA_0
#define PIN_LED2 -1
#define PIN_LOG0 -1 // sig_write
#define PIN_LOG1 -1
#define PIN_LOG2 -1 // sig error
#define PIN_LOG3 -1

#define PIN_PWR PA_11
#define PIN_P0 PA_2
#define PIN_P1 PA_3
#define PIN_ASCK PA_5
#define PIN_AMOSI PA_7

#define PIN_SERVO PA_6
/*
#define PIN_GLO_SENSE0 PA_1
#define PIN_GLO_SENSE1 PA_4
*/

#define PIN_GLO0 PB_0
#define PIN_GLO1 PB_1

#define PIN_ACC_VCC PB_4
#define PIN_ACC_MISO PB_6
#define PIN_ACC_MOSI PB_5
#define PIN_ACC_SCK PB_7
#define PIN_ACC_CS PB_8
//#define PIN_ACC_CS -1
#define ACC_PORT GPIOB

#define UART_PIN PA_9
#define UART_PIN_AF LL_GPIO_AF_1
#define USART_IDX 1

#define OUTPUT_PINS                                                                                \
    PIN_LOG0, PIN_LOG1, PIN_LOG2, PIN_LOG3, PIN_LED, PIN_LED2, PIN_PWR, PIN_P0, PIN_P1,            \
        PIN_LED_GND, PIN_GLO0, PIN_GLO1, PIN_ACC_MOSI, PIN_ACC_SCK, PIN_ACC_VCC, PIN_ACC_CS,       \
        PIN_ASCK, PIN_AMOSI, PIN_SERVO,