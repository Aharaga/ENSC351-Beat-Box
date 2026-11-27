#ifndef LED_H
#define LED_H
#include <stdbool.h>

// Initialize the LED module
void led_init(void);

// Clean up the LED module (ensure both LEDs are off at shutdown)
void led_cleanup(void);

// Control individual LEDs (e.g., green or red)
void led_set_state(bool green_on, bool red_on);

#endif