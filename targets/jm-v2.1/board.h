#define PIN_LED PA_1

#define UART_PIN PA_9
#define UART_PIN_AF LL_GPIO_AF_1
#define USART_IDX 1

// needed by light service
#define PIN_PWR PA_10
#define PIN_AMOSI PA_7
#define PIN_ASCK PA_5

#ifndef BL
#define DEVICE_DMESG_BUFFER_SIZE 832
#endif