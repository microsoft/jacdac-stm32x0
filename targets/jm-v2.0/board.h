#define PIN_LED PA_1
#define PIN_LED_GND PA_0

#define PIN_PWR PA_10

#define PIN_AMOSI PA_7
#define PIN_ASCK PA_5

#define UART_PIN PA_9
#define UART_PIN_AF LL_GPIO_AF_1
#define USART_IDX 1

#define BUZZER_OFF 0

#ifndef BL
#define DEVICE_DMESG_BUFFER_SIZE 832
#endif

// we don't actually use it, but let's just make sure it's built somewhere
#define ACC_QMA7981
