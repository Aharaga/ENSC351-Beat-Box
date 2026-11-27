#include "led.h"
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>

static bool is_initialized = false;

static void write_led_brightness(const char *path, bool on)
{
    FILE *file = fopen(path, "w");
    if (file != NULL) {
        fprintf(file, "%d", on ? 255 : 0);
        fflush(file);
        fclose(file);
    }
}

static void set_trigger(const char *path, const char *trigger)
{
    FILE *file = fopen(path, "w");
    if (file != NULL) {
        fprintf(file, "%s", trigger);
        fflush(file);
        fclose(file);
    }
}

void led_init(void)
{
    printf("LED - Initializing\n");
    assert(!is_initialized);
    is_initialized = true;

    // Disable default kernel triggers on both LED nodes
    set_trigger("/sys/class/leds/ACT/trigger", "none");
    set_trigger("/sys/class/leds/PWR/trigger", "none");

    led_set_state(false, false);
}

void led_set_state(bool green_on, bool red_on)
{
    assert(is_initialized);

    // Control Green LED (ACT)
    write_led_brightness("/sys/class/leds/ACT/brightness", green_on);
    
    // Control Red LED (PWR)
    write_led_brightness("/sys/class/leds/PWR/brightness", red_on);
}

void led_cleanup(void)
{
    printf("LED - Cleanup\n");
    assert(is_initialized);

    led_set_state(false, false);
    is_initialized = false;
}