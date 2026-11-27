
#include "button.h"
#include <stdio.h>
#include <gpiod.h>
#include <time.h>

#define DEBOUNCE_MS 30

// GPIO chips and line numbers for each button
static const char *chipPaths[BUTTON_COUNT] = {
    "/dev/gpiochip2",
    "/dev/gpiochip2",
    "/dev/gpiochip1",
    "/dev/gpiochip1"
};

static const unsigned int pins[BUTTON_COUNT] = {
    33, 41, 7, 10
};

static struct gpiod_chip *chips[BUTTON_COUNT];
static struct gpiod_line_request *requests[BUTTON_COUNT];

static bool lastReading[BUTTON_COUNT];
static bool stableState[BUTTON_COUNT];
static long long lastChange[BUTTON_COUNT];

// Get the current time in milliseconds
static long long getTimeMs(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
}

bool button_init(void)
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        chips[i] = gpiod_chip_open(chipPaths[i]);

        if (!chips[i]) {
            perror("Failed to open GPIO chip");
            button_cleanup();
            return false;
        }

        struct gpiod_line_settings *settings =
            gpiod_line_settings_new();

        struct gpiod_line_config *config =
            gpiod_line_config_new();

        if (!settings || !config) {
            gpiod_line_settings_free(settings);
            gpiod_line_config_free(config);
            button_cleanup();
            return false;
        }

        // Set button as an input with a pull-up
        gpiod_line_settings_set_direction(
            settings, GPIOD_LINE_DIRECTION_INPUT);

        gpiod_line_settings_set_bias(
            settings, GPIOD_LINE_BIAS_PULL_UP);

        if (gpiod_line_config_add_line_settings(
            config, &pins[i], 1, settings) == 0) {

            requests[i] = gpiod_chip_request_lines(
                chips[i], NULL, config);
        }

        gpiod_line_config_free(config);
        gpiod_line_settings_free(settings);

        if (!requests[i]) {
            perror("Failed to request button GPIO");
            button_cleanup();
            return false;
        }

        lastReading[i] = false;
        stableState[i] = false;
        lastChange[i] = getTimeMs();
    }

    printf("All four buttons initialized\n");
    return true;
}

bool button_is_button_pressed(Button button)
{
    if (button < 0 || button >= BUTTON_COUNT ||
        !requests[button]) {
        return false;
    }

    // Buttons are active low
    return gpiod_line_request_get_value(
        requests[button], pins[button]) == 0;
}

bool button_was_pressed(Button button)
{
    if (button < 0 || button >= BUTTON_COUNT ||
        !requests[button]) {
        return false;
    }

    bool reading = button_is_button_pressed(button);
    long long now = getTimeMs();

    // Restart debounce timer when the reading changes
    if (reading != lastReading[button]) {
        lastChange[button] = now;
        lastReading[button] = reading;
    }

    // Only accept the change if it stays stable
    if (now - lastChange[button] >= DEBOUNCE_MS) {
        if (reading != stableState[button]) {
            stableState[button] = reading;

            // Only trigger when the button is pressed
            return reading;
        }
    }

    return false;
}

void button_cleanup(void)
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        if (requests[i]) {
            gpiod_line_request_release(requests[i]);
            requests[i] = NULL;
        }

        if (chips[i]) {
            gpiod_chip_close(chips[i]);
            chips[i] = NULL;
        }
    }

    printf("Buttons cleaned up\n");
}
