// Specify where the status LED is connected; the define is required but can be set to -1 to disable LED
// The pin is active-high (ie., the connection is MCU -> LED + resistor -> GND)
#define PIN_LED PA_1

// If this board controls a power supply (either to the JACDAC bus or to an external consumer),
// PIN_PWR should be connected an active-low enable pin of power controller
// (typically just a P-MOSFET), with an external pull-up.
// Function power_pin_enable() is then used to enable/disable power.
// #define PIN_PWR PA_10

// These are application logging pins. They are often used as pin_pulse(PIN_P0, 3); or similar.
// If you have spare pins, you can expose them with test pads, and use pin_pulse() etc. in your
// service implementations.
// #define PIN_P0 PF_0
// #define PIN_P1 PF_1

// If your board is using I2C, uncomment the section below.
// #define PIN_SDA PA_10
// #define PIN_SCL PA_9
// #define I2C_AF LL_GPIO_AF_4

// If you're using accelerometer service, and the INT pin of accelerometer is connected (it should be!)
// define the pin below.
// #define PIN_ACC_INT PA_7

// Configuration of JACDAC interface
#define USART_IDX 1 // only USART1 available on STM32F030
// alternative function for the pin
#define UART_PIN_AF LL_GPIO_AF_1
// The JACDAC pin. It's best to use 5V-tolerant pin (ie., PA_9), but if that is used by I2C then use PA_2.
#define UART_PIN PA_2
// #define UART_PIN PA_9

// Enable JACDAC Logger (console) service for debugging.
// #define JD_CONSOLE 1

// All i2c addresses are the 7-bit addresses.

// Define the accelerometer used by the board if any
// #define ACC_KXTJ3 // Kionix Inc. KXTJ3-1057; I2C addr 0x0E
// #define ACC_QMA7981 // QST QMA7981; I2C addr 0x12

// Override accelerometer I2C address if needed.
// #define ACC_I2C_ADDR 0x0E

// These two are used by px_init() (for RGB LED service), or alternatively dspi_init().
// PA_5/7 are the only pins supported.
// #define PIN_AMOSI PA_7
// #define PIN_ASCK PA_5

// When using dspi_init(), you can also enable MISO pin, by uncommenting the lines below.
// #define PIN_AMISO PA_6
// #define SPI_RX 1

// Set to 1, if buzzer is off, when buzzer pin set high. Set to 0 otherwise.
// Best use pin PA_10 or PA_4 for sound (in buzzer_init()).
// #define BUZZER_OFF 1

// Temperature (and humidity) sensor type.
// #define TEMP_TH02 1

// Default is I2C_FAST_MODE=1 - 400kHz mode
// Set to 0 to enable 100kHz mode
// #define I2C_FAST_MODE 0
