#ifndef JD_USER_CONFIG_H
#define JD_USER_CONFIG_H

#define JD_CONFIG_TEMPERATURE 1

#include "dmesg.h"
#include "board.h"

#define JD_LOG DMESG

#ifndef PIN_LED_GND
#define PIN_LED_GND -1
#endif

// secondary LED
#ifndef PIN_LED2
#define PIN_LED2 -1
#endif

// logging pins for JD implementation
#ifndef PIN_LOG0
#define PIN_LOG0 -1 // sig_write
#endif
#ifndef PIN_LOG1
#define PIN_LOG1 -1
#endif
#ifndef PIN_LOG2
#define PIN_LOG2 -1 // sig error
#endif
#ifndef PIN_LOG3
#define PIN_LOG3 -1
#endif

// logging pins for application
#ifndef PIN_P0
#define PIN_P0 -1
#endif
#ifndef PIN_P1
#define PIN_P1 -1
#endif

#ifndef PIN_PWR
#define PIN_PWR -1
#endif

#endif
